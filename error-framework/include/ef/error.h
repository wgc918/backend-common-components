/**
 * @file    error.h
 * @brief   运行时错误对象 — 框架核心类型
 *
 * Error 是框架中最常用的错误载体。设计要点：
 *   - 使用 int code 标识错误类型（非 enum class），支持模块扩展。
 *   - 小对象优化（SBO）：≤ 64 字节上下文信息存储在栈上，避免堆分配。
 *   - 深拷贝语义：operator= 和拷贝构造会递归复制错误链。
 *   - 支持错误链：caused_by() 可将底层错误附加为原因。
 *   - 提供外部错误注册表，允许模块动态注册错误元数据。
 *
 * @author  backend-common-components
 * @date    2026-06-03
 */

#pragma once

#include "error_kind.h"
#include "source_location.h"
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ef {

// ============================================================================
// 外部错误注册表（模块扩展点）
// ============================================================================

/**
 * @brief 外部错误注册表回调类型
 *
 * 模块可实现此函数来扩展错误元数据查询。
 * 返回 nullptr 表示非本模块管理的错误码。
 *
 * @note 框架内部优先查内置表，未命中再查外部注册。
 *       模块应在自己的初始化代码中调用 SetExternalLookup 设置回调。
 */
using ExternalLookupFn = const ErrorInfo* (*)(int code);

/**
 * @brief  获取 / 设置外部错误查找回调
 * @return 当前注册的回调函数指针（默认为 nullptr）
 */
inline ExternalLookupFn& GetExternalLookup() {
    static ExternalLookupFn fn = nullptr;
    return fn;
}

/**
 * @brief  设置外部错误查找回调
 * @param  fn  回调函数，接受错误码，返回 ErrorInfo 或 nullptr
 *
 * 使用示例（在模块初始化时调用）：
 * @code
 *   ef::SetExternalLookup([](int code) -> const ef::ErrorInfo* {
 *       return db::LookupDbError(code);
 *   });
 * @endcode
 */
inline void SetExternalLookup(ExternalLookupFn fn) {
    GetExternalLookup() = fn;
}

/**
 * @brief 外部查表（供 error_kind.h 中 LookupError 调用）
 */
inline const ErrorInfo* LookupExternalError(int code) noexcept {
    auto fn = GetExternalLookup();
    return fn ? fn(code) : nullptr;
}

// ============================================================================
// Error — 运行时错误对象
// ============================================================================

/**
 * @class Error
 * @brief 运行时错误对象，携带错误码、源码位置、上下文信息及可选的错误链
 *
 * 典型构造方式：
 * @code
 *   return EF_ERROR(errors::kNotFound, "file '%s' not found", path);
 *   return EF_MAKE(errors::kTimeout, "request took %d ms", elapsed);
 * @endcode
 *
 * 访问方式：
 * @code
 *   Error err = ...;
 *   int code = err.code();          // 错误码
 *   const char* what = err.what();  // 错误描述（来自查表）
 *   Severity sev = err.severity();  // 严重程度
 *   std::string msg = err.message();// 上下文消息
 * @endcode
 */
class Error {
public:
    /** @brief 小对象优化栈缓冲区大小（字节） */
    static constexpr size_t kSboSize = 64;

    // ========================================================================
    // 构造 / 拷贝 / 移动
    // ========================================================================

    /**
     * @brief  带格式化的错误构造（printf 风格）
     *
     * @tparam Args  格式化参数类型包
     * @param  code  错误码（框架内置或模块自定义）
     * @param  loc   源码位置（由 EF_SOURCE_LOC() 或便捷宏自动填充）
     * @param  fmt   printf 风格格式化字符串
     * @param  args  格式化参数
     *
     * @note  格式化结果 ≤ kSboSize-1 时完全栈上操作，零堆分配。
     */
    template <typename... Args>
    Error(int code, SourceLocation loc, const char* fmt, Args&&... args)
        : code_(code)
        , location_(loc)
        , cause_(nullptr)
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
        int n = std::snprintf(sbo_, kSboSize, fmt, std::forward<Args>(args)...);
        if (n >= static_cast<int>(kSboSize)) {
            std::vector<char> tmp(n + 1);
            std::snprintf(tmp.data(), tmp.size(), fmt, std::forward<Args>(args)...);
            heap_ = std::string(tmp.data());
            sbo_[0] = '\0';
        }
#pragma GCC diagnostic pop
    }

    /**
     * @brief  仅错误码构造（无附加上下文）
     * @param  code  错误码
     * @param  loc   源码位置
     */
    Error(int code, SourceLocation loc)
        : code_(code)
        , location_(loc)
        , cause_(nullptr)
    {
        sbo_[0] = '\0';
    }

    /**
     * @brief 深拷贝构造，递归复制错误链
     */
    Error(const Error& other)
        : code_(other.code_)
        , location_(other.location_)
    {
        std::memcpy(sbo_, other.sbo_, kSboSize);
        heap_ = other.heap_;
        if (other.cause_) {
            cause_ = std::make_unique<Error>(*other.cause_);
        }
    }

    /**
     * @brief 深拷贝赋值，递归复制错误链
     *
     * @note 自赋值安全。
     */
    Error& operator=(const Error& other) {
        if (this != &other) {
            code_     = other.code_;
            location_ = other.location_;
            std::memcpy(sbo_, other.sbo_, kSboSize);
            heap_  = other.heap_;
            cause_ = other.cause_ ? std::make_unique<Error>(*other.cause_) : nullptr;
        }
        return *this;
    }

    /** @brief 移动构造（noexcept） */
    Error(Error&&) noexcept = default;

    /** @brief 移动赋值（noexcept） */
    Error& operator=(Error&&) noexcept = default;

    // ========================================================================
    // 错误链
    // ========================================================================

    /**
     * @brief  附加底层错误作为原因，支持错误链追踪
     *
     * @param  cause  底层错误对象（右值，所有权转移）
     * @return 自身引用，支持链式调用
     *
     * 使用示例：
     * @code
     *   auto low  = EF_ERROR(errors::kConnectionRefused, "connect to %s:%d failed", host, port);
     *   auto high = EF_ERROR(errors::kTimeout, "retry exhausted after %d tries", 3)
     *                   .caused_by(std::move(low));
     * @endcode
     */
    Error& caused_by(Error&& cause) {
        cause_ = std::make_unique<Error>(std::move(cause));
        return *this;
    }

    /**
     * @brief  是否有错误链下层原因
     */
    bool has_cause() const noexcept { return cause_ != nullptr; }

    // ========================================================================
    // 访问器（均 noexcept）
    // ========================================================================

    /** @brief 返回错误码 */
    int             code()      const noexcept { return code_; }

    /** @brief 返回错误发生的源码位置 */
    SourceLocation  location()  const noexcept { return location_; }

    /** @brief 返回错误严重程度（由查表获得，未注册则返回 Error） */
    Severity        severity()  const noexcept { return LookupError(code_)->severity; }

    /** @brief 返回错误名称（由查表获得，未注册则返回 "Unknown"） */
    const char*     what()      const noexcept { return LookupError(code_)->name; }

    /** @brief 返回错误描述（由查表获得） */
    const char*     description() const noexcept { return LookupError(code_)->message; }

    /**
     * @brief  返回错误链的下层原因
     * @return 指向下层 Error 的指针，不存在则返回 nullptr
     */
    const Error*    cause()     const noexcept { return cause_.get(); }

    /**
     * @brief  是否为成功状态（code == 0）
     */
    bool            is_ok()     const noexcept { return code_ == 0; }

    // ========================================================================
    // 消息格式化
    // ========================================================================

    /**
     * @brief  获取格式化后的上下文消息（运行时的附加信息）
     *
     * @return std::string，优先返回栈缓冲区内容，否则返回堆上字符串
     *
     * @note  区别于 what()/description()（来自注册表的静态描述），
     *        message() 是 Error 构造时传入的动态上下文信息。
     */
    std::string message() const {
        return (sbo_[0] != '\0') ? std::string(sbo_) : heap_;
    }

    /**
     * @brief  是否有附加上下文消息
     */
    bool has_message() const noexcept {
        return sbo_[0] != '\0' || !heap_.empty();
    }

    // ========================================================================
    // 完整格式化输出
    // ========================================================================

    /**
     * @brief  返回完整格式化的错误字符串（含错误链）
     *
     * 格式：[Severity] what (code) @ file:line | message
     *        caused by: ...
     *
     * @return  格式化后的完整字符串
     */
    std::string to_string() const {
        return format_chain(0);
    }

private:
    /**
     * @brief 递归格式化错误链
     * @param depth  当前深度（控制缩进）
     */
    std::string format_chain(int depth) const {
        auto* info = LookupError(code_);
        std::string result;
        if (depth > 0) {
            result += "\n  caused by: ";
        }
        result += info->name;
        result += " (" + std::to_string(code_) + ")";
        result += " @ " + std::string(location_.file) + ":" + std::to_string(location_.line);

        if (has_message()) {
            result += " | ";
            result += message();
        }

        if (cause_) {
            result += cause_->format_chain(depth + 1);
        }
        return result;
    }

    int                     code_;       ///< 错误码
    SourceLocation          location_;   ///< 源码位置
    char                    sbo_[kSboSize]{};  ///< SBO 栈缓冲区
    std::string             heap_;       ///< 堆存储（长消息溢出时使用）
    std::unique_ptr<Error>  cause_;      ///< 错误链下层原因（可选）
};

}  // namespace ef