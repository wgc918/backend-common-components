//-----------------------------------------------------------------------------
// 版权所有 (C) 2026 backend-common-components 项目
//
// 文件: result.h
// 作者: wgc
// 创建日期: 2026年6月
// 最后修改: 2026年6月
//
// 描述:
//     Result<T, E> — tagged union，成功值或错误二选一，基于 placement new 实现。
//     提供 Result<void, E> 特化用于无返回值的场景。
//
// 功能特性:
//     - Result<T, E>  泛型结果类型，统一成功值 T 和错误 E 的传递
//     - Result<void, E>  无值特化版本
//     - is_ok() / is_err() / operator bool()  状态查询
//     - unwrap() / unwrap_move() / unwrap_err()  值提取
//     - value_or()  提供默认值的安全提取
//     - operator-> / operator*  指针/解引用访问
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
#include <cstdlib>
#include <new>
#include <type_traits>
#include <utility>

#include "error.h"

/// @brief 泛型结果类型 — tagged union，同时只能持有成功值 T 或错误 E
/// @tparam T  成功时的值类型
/// @tparam E  错误类型
template <typename T, typename E>
class Result
{
public:
    // ============================================
    // 构造函数与析构函数
    // ============================================

    /// @brief 从成功值拷贝构造
    /// @param val  成功值
    Result(const T& val);

    /// @brief 从成功值移动构造
    /// @param val  成功值（右值）
    Result(const T&& val) noexcept;

    /// @brief 从错误值拷贝构造
    /// @param err  错误值
    Result(const E& err);

    /// @brief 从错误值移动构造
    /// @param err  错误值（右值）
    Result(E&& err) noexcept;

    /// @brief 移动构造函数
    /// @param other  源对象
    Result(Result&& other) noexcept;

    /// @brief 移动赋值操作符
    /// @param other  源对象
    /// @return *this
    Result& operator=(Result&& other) noexcept;

    /// @brief 拷贝构造 — 删除，避免意外拷贝
    Result(const Result& other) = delete;

    /// @brief 拷贝赋值 — 删除，避免意外拷贝
    Result& operator=(const Result& other) = delete;

    /// @brief 析构 — 根据状态销毁 T 或 E
    ~Result();

    // ============================================
    // 状态查询
    // ============================================

    /// @brief 是否为成功状态
    bool is_ok() const noexcept;

    /// @brief 是否为错误状态
    bool is_err() const noexcept;

    /// @brief 隐式 bool 转换，成功时为 true
    explicit operator bool() const noexcept;

    // ============================================
    // 值提取
    // ============================================

    /// @brief 提取成功值（const），错误时 abort
    const T& unwrap() const;

    /// @brief 提取成功值，错误时 abort
    T& unwrap();

    /// @brief 移动提取成功值，错误时 abort
    T unwrap_move();

    /// @brief 提取错误值（const）
    const E& unwrap_err() const;

    /// @brief 提取错误值
    E& unwrap_err();

    /// @brief 提取成功值，错误时返回默认值（右值版本）
    /// @param val  默认值
    T value_or(T&& val);

    /// @brief 提取成功值，错误时返回默认值（const 版本）
    /// @param val  默认值
    T value_or(const T& val);

    // ============================================
    // 指针/解引用访问
    // ============================================

    /// @brief 指针访问成功值，错误时 abort
    T* operator->();

    /// @brief 指针访问成功值（const），错误时 abort
    const T* operator->() const;

    /// @brief 解引用访问成功值，错误时 abort
    T& operator*();

    /// @brief 解引用访问成功值（const），错误时 abort
    const T& operator*() const;

private:
    static constexpr size_t kSize  = sizeof(T) > sizeof(E) ? sizeof(T) : sizeof(E);
    static constexpr size_t kAlign = alignof(T) > alignof(E) ? alignof(T) : alignof(E);
    bool                    m_ok;                ///< 状态标记：true=成功，false=错误
    alignas(kAlign) unsigned char m_buf[kSize];  ///< 原始存储，placement new 构造 T 或 E
};

// ------------------------------ Result<T, E> 构造函数实现 ------------------------------

template <typename T, typename E>
Result<T, E>::Result(const T& val) : m_ok(true)
{
    new (&m_buf) T(val);
}

template <typename T, typename E>
Result<T, E>::Result(const T&& val) noexcept : m_ok(true)
{
    new (&m_buf) T(val);
}

template <typename T, typename E>
Result<T, E>::Result(const E& err) : m_ok(false)
{
    new (&m_buf) E(err);
}

template <typename T, typename E>
Result<T, E>::Result(E&& err) noexcept : m_ok(false)
{
    new (&m_buf) E(std::move(err));
}

template <typename T, typename E>
Result<T, E>::Result(Result&& other) noexcept : m_ok(other.m_ok)
{
    if (m_ok)
        new (&m_buf) T(std::move(*reinterpret_cast<T*>(&other.m_buf)));
    else
        new (&m_buf) E(std::move(*reinterpret_cast<E*>(&other.m_buf)));
}

template <typename T, typename E>
Result<T, E>& Result<T, E>::operator=(Result&& other) noexcept
{
    if (this == &other)
        return *this;

    m_ok = other.m_ok;
    if (m_ok)
    {
        reinterpret_cast<T*>(&m_buf)->~T();
        new (&m_buf) T(std::move(*reinterpret_cast<T*>(&other.m_buf)));
    }
    else
    {
        reinterpret_cast<E*>(&m_buf)->~E();
        new (&m_buf) E(std::move(*reinterpret_cast<E*>(&other.m_buf)));
    }

    return *this;
}

template <typename T, typename E>
Result<T, E>::~Result()
{
    if (m_ok)
        reinterpret_cast<T*>(&m_buf)->~T();
    else
        reinterpret_cast<E*>(&m_buf)->~E();
}

// ------------------------------ Result<T, E> 状态查询实现 ------------------------------

template <typename T, typename E>
inline bool Result<T, E>::is_ok() const noexcept
{
    return m_ok;
}

template <typename T, typename E>
inline bool Result<T, E>::is_err() const noexcept
{
    return !is_ok();
}

template <typename T, typename E>
inline Result<T, E>::operator bool() const noexcept
{
    return is_ok();
}

// ------------------------------ Result<T, E> 值提取实现 ------------------------------

template <typename T, typename E>
inline const T& Result<T, E>::unwrap() const
{
    if (!m_ok)
        std::abort();
    return *reinterpret_cast<T*>(&m_buf);
}

template <typename T, typename E>
inline T& Result<T, E>::unwrap()
{
    if (!m_ok)
        std::abort();
    return *reinterpret_cast<T*>(&m_buf);
}

template <typename T, typename E>
inline T Result<T, E>::unwrap_move()
{
    if (!m_ok)
        std::abort();
    return std::move(*reinterpret_cast<T*>(&m_buf));
}

template <typename T, typename E>
inline const E& Result<T, E>::unwrap_err() const
{
    return *reinterpret_cast<E*>(&m_buf);
}

template <typename T, typename E>
inline E& Result<T, E>::unwrap_err()
{
    return *reinterpret_cast<E*>(&m_buf);
}

template <typename T, typename E>
inline T Result<T, E>::value_or(T&& val)
{
    return m_ok ? std::move(*reinterpret_cast<T*>(&m_buf)) : std::forward<T>(val);
}

template <typename T, typename E>
inline T Result<T, E>::value_or(const T& val)
{
    return m_ok ? *reinterpret_cast<T*>(&m_buf) : val;
}

// ------------------------------ Result<T, E> 指针/解引用实现 ------------------------------

template <typename T, typename E>
inline T* Result<T, E>::operator->()
{
    if (!m_ok)
        std::abort();
    return reinterpret_cast<T*>(&m_buf);
}

template <typename T, typename E>
inline const T* Result<T, E>::operator->() const
{
    if (!m_ok)
        std::abort();
    return reinterpret_cast<T*>(&m_buf);
}

template <typename T, typename E>
inline T& Result<T, E>::operator*()
{
    return unwrap();
}

template <typename T, typename E>
inline const T& Result<T, E>::operator*() const
{
    return unwrap();
}

// ============================================
// Result<void, E> 特化
// ============================================

/// @brief Result 的 void 特化 — 无成功值，仅表示成功或错误状态
/// @tparam E  错误类型
template <typename E>
class Result<void, E>
{
public:
    // ============================================
    // 构造函数与析构函数
    // ============================================

    /// @brief 默认构造 — 成功状态
    Result();

    /// @brief 从错误值拷贝构造
    /// @param err  错误值
    Result(const E& err);

    /// @brief 从错误值移动构造
    /// @param err  错误值（右值）
    Result(E&& err) noexcept;

    /// @brief 移动构造函数
    /// @param other  源对象
    Result(Result&& other) noexcept;

    /// @brief 移动赋值操作符
    /// @param other  源对象
    /// @return *this
    Result& operator=(Result&& other) noexcept;

    /// @brief 拷贝构造 — 删除，避免意外拷贝
    Result(const Result& other) = delete;

    /// @brief 拷贝赋值 — 删除，避免意外拷贝
    Result& operator=(const Result& other) = delete;

    /// @brief 析构 — 根据状态销毁 E
    ~Result();

    // ============================================
    // 工厂方法
    // ============================================

    /// @brief 创建成功状态的 Result<void, E>
    Result OK();

    // ============================================
    // 状态查询
    // ============================================

    /// @brief 是否为成功状态
    bool is_ok() const noexcept;

    /// @brief 是否为错误状态
    bool is_err() const noexcept;

    /// @brief 隐式 bool 转换，成功时为 true
    explicit operator bool() const noexcept;

    // ============================================
    // 错误值提取
    // ============================================

    /// @brief 提取错误值（const）
    const E& unwrap_err() const;

    /// @brief 提取错误值
    E& unwrap_err();

private:
    bool m_ok;                                           ///< 状态标记：true=成功，false=错误
    alignas(alignof(E)) unsigned char m_buf[sizeof(E)];  ///< 原始存储
};

// ------------------------------ Result<void, E> 实现 ------------------------------

template <typename E>
Result<void, E>::Result() : m_ok(true)
{
}

template <typename E>
Result<void, E>::Result(const E& err) : m_ok(false)
{
    new (&m_buf) E(err);
}

template <typename E>
Result<void, E>::Result(E&& err) noexcept : m_ok(false)
{
    new (&m_buf) E(std::move(err));
}

template <typename E>
Result<void, E>::Result(Result&& other) noexcept : m_ok(other.m_ok)
{
    new (&m_buf) E(std::move(*reinterpret_cast<E*>(&other.m_buf)));
}

template <typename E>
Result<void, E>& Result<void, E>::operator=(Result&& other) noexcept
{
    if (this == &other)
        return *this;
    new (&m_buf) E(std::move(*reinterpret_cast<E*>(&other.m_buf)));
    m_ok = other.m_ok;
    return *this;
}

template <typename E>
Result<void, E>::~Result()
{
    if (!m_ok)
        reinterpret_cast<E*>(&m_buf)->~E();
}

template <typename E>
inline Result<void, E> Result<void, E>::OK()
{
    Result<void, E> ret;
    ret.m_ok = true;
    return ret;
}

template <typename E>
bool Result<void, E>::is_ok() const noexcept
{
    return m_ok;
}

template <typename E>
bool Result<void, E>::is_err() const noexcept
{
    return !is_ok();
}

template <typename E>
Result<void, E>::operator bool() const noexcept
{
    return is_ok();
}

template <typename E>
const E& Result<void, E>::unwrap_err() const
{
    return *reinterpret_cast<E*>(&m_buf);
}

template <typename E>
E& Result<void, E>::unwrap_err()
{
    return *reinterpret_cast<E*>(&m_buf);
}