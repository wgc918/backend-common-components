//-----------------------------------------------------------------------------
// 版权所有 (C) 2026 backend-common-components 项目
//
// 文件: error.h
// 作者: wgc
// 创建日期: 2026年6月
// 最后修改: 2026年6月
//
// 描述:
//     运行时错误对象 — 框架核心类型。
//     小对象优化（SBO）：≤ 64 字节上下文信息存储在栈上，避免堆分配。
//
// 功能特性:
//     - Error 类：携带错误码、源码位置、上下文信息，支持级联原因
//     - 小对象优化（SBO）：短消息存储在栈缓冲区，避免堆分配
//     - 内置错误码查找：code() / name() / what() / level() 自动查表
//     - 外部错误注册：支持业务模块注册自定义错误表
//
// 许可证:
//     MIT License
//
//     版权所有 (c) 2026 wgc
//
//     特此免费授予获得本软件副本和相关文档文件（以下简称"软件"）的任何人以处理软件的权利，
//     包括但不限于使用、复制、修改、合并、出版、分发、再许可和/或出售软件副本，
//     以及允许软件适用者这样做，须在下列条件下：
//
//     上述版权声明和本许可声明应包含在软件的所有副本或实质性部分中。
//
//     软件按"原样"提供，不提供任何形式的明示或暗示的保证，
//     包括但不限于对适销性、特定用途适用性和非侵权性的保证。
//     在任何情况下，作者或版权持有人均不对任何索赔、损害或其他责任负责，
//     无论是在合同诉讼、侵权诉讼或其他诉讼中，
//     由于软件或软件的使用或其他交易产生的。
//-----------------------------------------------------------------------------

#pragma once

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "error_kind.h"
#include "utils.h"

namespace ef
{

/// @brief 外部错误查表函数指针类型
using ExternalLookupFunc = const ErrorInfo* (*)(int code);

/// @brief 获取全局外部查表函数引用
ExternalLookupFunc& getExternalLookupFunc();

/// @brief 设置全局外部查表函数
/// @param func  指向查表函数的指针
void setExternalLookup(ExternalLookupFunc func);

/// @brief 调用外部查表函数查找错误码
/// @param code  错误码
/// @return 指向 ErrorInfo 的指针，未注册或未找到则返回 nullptr
const ErrorInfo* LookupExternalError(int code) noexcept;

/// @brief 运行时错误对象，携带错误码、源码位置与上下文信息
/// @details
///     采用小对象优化（SBO）：≤ 64 字节的消息存储在栈缓冲区 m_sbo_buf 中，
///     超出则分配堆内存。支持通过 caused_by() 级联底层错误。
class Error
{
public:
    // ============================================
    // 构造函数与析构函数
    // ============================================

    /// @brief 构造带格式化上下文信息的错误对象
    /// @tparam Args  格式化参数类型包
    /// @param code   错误码
    /// @param loc    源码位置信息
    /// @param fmt    格式化字符串
    /// @param args   格式化参数
    template <typename... Args>
    Error(int code, SourceLocation loc, const char* fmt, Args&&... args);

    /// @brief 构造不带上下文信息的错误对象
    /// @param code  错误码
    /// @param loc   源码位置信息
    Error(int code, SourceLocation loc);

    // ============================================
    // 拷贝与移动
    // ============================================

    /// @brief 拷贝构造函数
    /// @param other  源对象
    Error(const Error& other);

    /// @brief 拷贝赋值操作符
    /// @param other  源对象
    /// @return *this
    Error& operator=(const Error& other);

    /// @brief 移动构造函数
    /// @param other  源对象
    Error(Error&& other) noexcept;

    /// @brief 移动赋值操作符
    /// @param other  源对象
    /// @return *this
    Error& operator=(Error&& other) noexcept;

    // ============================================
    // 错误链接
    // ============================================

    /// @brief 设置级联原因错误
    /// @param cause  底层错误对象（右值）
    /// @return *this，支持链式调用
    Error& caused_by(Error&& cause);

    /// @brief 是否存在级联原因错误
    /// @return 有原因错误返回 true
    bool has_cause() const noexcept;

    /// @brief 获取级联原因错误指针
    /// @return 指向原因错误的指针，无原因则返回 nullptr
    const Error* cause() const noexcept;

    // ============================================
    // 错误信息访问
    // ============================================

    /// @brief 获取错误码
    int code() const noexcept;

    /// @brief 获取源码位置
    SourceLocation location() const noexcept;

    /// @brief 获取错误等级
    ErrorLevel level() const noexcept;

    /// @brief 获取错误码名称
    const char* name() const noexcept;

    /// @brief 获取错误码描述
    const char* what() const noexcept;

    /// @brief 获取用户自定义上下文消息
    std::string message() const;

    /// @brief 获取完整错误字符串（含级联）
    std::string to_string() const;

private:
    /// @brief 递归格式化错误字符串
    /// @param dept  级联深度（0 表示顶层错误）
    /// @return 格式化后的错误描述
    std::string format(int dept) const;

    static constexpr size_t kSboSize = 64;        ///< 小对象优化缓冲区大小
    int                     m_code;               ///< 错误码
    SourceLocation          m_location;           ///< 源码位置
    char                    m_sbo_buf[kSboSize];  ///< SBO 栈缓冲区
    std::string             m_heap_buf;           ///< 堆缓冲区（消息较长时使用）
    std::unique_ptr<Error>  m_cause;              ///< 级联原因错误
};

// ------------------------------ Error 模板构造函数实现 ------------------------------

template <typename... Args>
Error::Error(int code, SourceLocation loc, const char* fmt, Args&&... args)
    : m_code(code), m_location(loc), m_sbo_buf{}, m_heap_buf(), m_cause(nullptr)
{
    // 先尝试写入栈缓冲区
    int n = std::snprintf(m_sbo_buf, kSboSize, fmt, std::forward<Args>(args)...);
    if (n >= static_cast<int>(kSboSize))
    {
        // 栈缓冲区不够，使用堆缓冲区
        std::vector<char> temp(n + 1);
        std::snprintf(temp.data(), temp.size(), fmt, std::forward<Args>(args)...);
        m_heap_buf   = std::string(temp.data());
        m_sbo_buf[0] = '\0';
    }
}

}  // namespace ef