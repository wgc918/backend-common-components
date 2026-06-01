/**
 * @file    error.h
 * @brief   运行时错误对象 —— 框架核心类型
 *
 * Error 是框架中最常用的错误载体。设计要点：
 *   - 非模板类，ErrorKind 为运行时成员（而非编译期模板参数），降低复杂度。
 *   - 小对象优化（SBO）：≤ 64 字节的错误信息存储在栈上，避免堆分配。
 *   - 深拷贝语义：operator= 和拷贝构造会递归复制错误链。
 *   - 支持错误链：caused_by() 可将底层错误附加为原因。
 *
 * @author  backend-common-components
 * @date    2026-05-31
 * @version 1.0.0
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

// ============================================================================
// Error — 运行时错误对象
// ============================================================================

/**
 * @class Error
 * @brief 运行时错误对象，携带错误类型、上下文信息、源码位置及可选的错误链
 *
 * 典型用法（通过 AEF 宏）：
 * @code
 *   return AEF(FileNotFound, "config '%s' not found", path);
 * @endcode
 *
 * 或手动构造：
 * @code
 *   Error err(ErrorKind::NetworkTimeout, SOURCE_LOC(), "timeout after %d ms", ms);
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
     * @param  kind  错误类型枚举值
     * @param  loc   源码位置（由 SOURCE_LOC() 或 AEF 宏自动填充）
     * @param  fmt   printf 风格格式化字符串
     * @param  args 格式化参数
     *
     * @note  格式化结果 ≤ 63 字节时完全栈上操作，不触发堆分配。
     *        超出则自动分配 std::string 存储。
     */
    template<typename... Args>
    Error(ErrorKind kind, SourceLocation loc, const char* fmt, Args&&... args)
        : kind_(kind), location_(loc), cause_(nullptr)
    {
        int n = std::snprintf(sbo_, kSboSize, fmt, args...);
        if (n >= static_cast<int>(kSboSize)) {
            std::vector<char> tmp(n + 1);
            std::snprintf(tmp.data(), tmp.size(), fmt, args...);
            heap_ = std::string(tmp.data());
            sbo_[0] = '\0';
        }
    }

    /**
     * @brief 深拷贝构造，递归复制错误链
     *
     * @param other 源 Error 对象（const 左值引用）
     */
    Error(const Error& other)
        : kind_(other.kind_), location_(other.location_)
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
     * @param  other 源 Error 对象（const 左值引用）
     * @return 自身引用，支持链式赋值
     *
     * @note 自赋值安全。
     */
    Error& operator=(const Error& other) {
        if (this != &other) {
            kind_     = other.kind_;
            location_ = other.location_;
            std::memcpy(sbo_, other.sbo_, kSboSize);
            heap_  = other.heap_;
            cause_ = other.cause_ ? std::make_unique<Error>(*other.cause_) : nullptr;
        }
        return *this;
    }

    /** @brief 移动构造（noexcept，编译器默认生成） */
    Error(Error&&) noexcept = default;

    /** @brief 移动赋值（noexcept，编译器默认生成） */
    Error& operator=(Error&&) noexcept = default;

    // ========================================================================
    // 错误链
    // ========================================================================

    /**
     * @brief  附加底层错误作为原因，支持错误链追踪
     *
     * @param  cause 底层错误对象（右值，所有权转移）
     * @return 自身引用，支持链式调用
     *
     * 使用示例：
     * @code
     *   auto low  = AEF(ConnectionRefused, "connect failed");
     *   auto high = AEF(NetworkTimeout, "retry exhausted").caused_by(std::move(low));
     * @endcode
     */
    Error& caused_by(Error&& cause) {
        cause_ = std::make_unique<Error>(std::move(cause));
        return *this;
    }

    // ========================================================================
    // 访问器（均 noexcept）
    // ========================================================================

    /** @brief 返回错误类型枚举值 */
    ErrorKind       kind()      const noexcept { return kind_; }

    /** @brief 返回错误发生的源码位置 */
    SourceLocation  location()  const noexcept { return location_; }

    /** @brief 返回错误等级（由查表获得） */
    ErrorLevel      level()     const noexcept { return LookupError(kind_)->level; }

    /** @brief 返回错误码（由查表获得） */
    int             code()      const noexcept { return LookupError(kind_)->code; }

    /** @brief 返回错误描述字面量（由查表获得） */
    const char*     what()      const noexcept { return LookupError(kind_)->message; }

    /**
     * @brief  返回错误链的下层原因
     * @return 指向下层 Error 的指针，不存在则返回 nullptr
     */
    const Error*    cause()     const noexcept { return cause_.get(); }

    // ========================================================================
    // 消息格式化
    // ========================================================================

    /**
     * @brief  获取格式化后的上下文消息
     *
     * @return std::string，优先返回栈缓冲区内容，否则返回堆上字符串的副本
     *
     * @note  每次调用会拷贝一份 std::string，高频场景可考虑直接使用 what()。
     */
    std::string message() const {
        return (sbo_[0] != '\0') ? std::string(sbo_) : heap_;
    }

private:
    ErrorKind               kind_;     /**< 错误类型枚举值               */
    SourceLocation          location_; /**< 源码位置（文件 / 行 / 函数） */
    char                    sbo_[kSboSize]{};  /**< 小对象优化栈缓冲区   */
    std::string             heap_;     /**< 长消息时的堆存储             */
    std::unique_ptr<Error>  cause_;    /**< 错误链的下层原因（可选）     */
};
