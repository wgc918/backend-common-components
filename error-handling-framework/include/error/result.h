/**
 * @file    result.h
 * @brief   Result<T, E> —— 双参数 tagged union，成功值或错误二选一
 *
 * 核心设计：
 *   - 双模板参数：T = 成功类型，E = 错误类型（默认 Error）。
 *   - 底层为 aligned storage + placement new 实现的 tagged union。
 *   - 禁止拷贝（避免隐式双份数据），仅支持移动。
 *   - 提供 unwrap() / unwrap_err() / value_or() 三种解包方式。
 *
 * 模块可传入自定义错误类型：
 * @code
 *   struct MyError { int code; };
 *   Result<int, MyError> DoWork();
 * @endcode
 *
 * @author  backend-common-components
 * @date    2026-05-31
 * @version 1.0.0
 */

#pragma once

#include "error.h"
#include <type_traits>
#include <new>

// ============================================================================
// Result<T, E> — tagged union
// ============================================================================

/**
 * @class Result
 * @brief 成功值或错误二选一的容器
 *
 * @tparam T  成功时的值类型
 * @tparam E  错误时的值类型，默认为 Error
 *
 * 典型用法：
 * @code
 *   Result<int> Div(int a, int b) {
 *       if (b == 0) return AEF(InvalidArg, "division by zero");
 *       return a / b;
 *   }
 *
 *   auto r = Div(10, 0);
 *   if (!r) { LogError(r.unwrap_err()); }
 *   else    { use(r.unwrap()); }
 * @endcode
 */
template<typename T, typename E = Error>
class Result {
public:
    // ========================================================================
    // 构造 / 析构 / 移动
    // ========================================================================

    /**
     * @brief 从成功值构造（const 左值）
     * @param val 成功值（拷贝）
     */
    Result(const T& val) : ok_(true) { new (&buf_) T(val); }

    /**
     * @brief 从成功值构造（右值，noexcept）
     * @param val 成功值（移动）
     */
    Result(T&& val) noexcept : ok_(true) { new (&buf_) T(std::move(val)); }

    /**
     * @brief 从错误构造（const 左值）
     * @param err 错误对象（拷贝）
     */
    Result(const E& err) : ok_(false) { new (&buf_) E(err); }

    /**
     * @brief 从错误构造（右值，noexcept）
     * @param err 错误对象（移动）
     */
    Result(E&& err) noexcept : ok_(false) { new (&buf_) E(std::move(err)); }

    /**
     * @brief 移动构造（noexcept）
     * @param o 源 Result（右值）
     */
    Result(Result&& o) noexcept : ok_(o.ok_) {
        if (ok_) new (&buf_) T(std::move(*o.template ptr<T>()));
        else     new (&buf_) E(std::move(*o.template ptr<E>()));
    }

    /** @brief 析构，根据当前状态调用 T 或 E 的析构函数 */
    ~Result() { destroy(); }

    /**
     * @brief 禁止拷贝构造
     *
     * Result 采用独占所有权语义，拷贝会导致数据重复且实现复杂。
     * 如需传递，请使用 std::move()。
     */
    Result(const Result&) = delete;

    /**
     * @brief 禁止拷贝赋值
     * @see Result(const Result&)
     */
    Result& operator=(const Result&) = delete;

    // ========================================================================
    // 状态判断
    // ========================================================================

    /** @brief 当前持有成功值 */
    bool is_ok()  const noexcept { return ok_; }

    /** @brief 当前持有错误 */
    bool is_err() const noexcept { return !ok_; }

    /**
     * @brief  隐式 bool 转换，方便 if 判断
     *
     * @code
     *   auto r = DoSomething();
     *   if (!r) { handle_error(r.unwrap_err()); }
     * @endcode
     */
    explicit operator bool() const noexcept { return ok_; }

    // ========================================================================
    // 解包
    // ========================================================================

    /**
     * @brief  解包成功值
     *
     * @return T& 成功值的引用
     *
     * @warning 若当前持有错误（!is_ok()），调用 std::abort() 终止程序。
     *          调用前务必通过 if (result) 或 is_ok() 确保安全。
     */
    T& unwrap() {
        if (!ok_) std::abort();
        return *ptr<T>();
    }

    /**
     * @brief  解包错误值（const）
     *
     * @return const E& 错误对象的常量引用
     *
     * @note 调用方需确保 is_err() 为 true，否则行为未定义（返回垃圾数据）。
     */
    const E& unwrap_err() const { return *cptr<E>(); }

    /**
     * @brief  错误时返回默认值（移动语义）
     *
     * @param  def 默认值（右值）
     * @return 成功时移动 T，错误时移动 def
     *
     * 使用示例：
     * @code
     *   int port = LoadConfig("server.json").value_or(ServerConfig{8080}).port;
     * @endcode
     */
    T value_or(T&& def) && {
        return ok_ ? std::move(*ptr<T>()) : std::forward<T>(def);
    }

private:
    // ========================================================================
    // 内部辅助
    // ========================================================================

    /** @brief 将存储区 reinterpret_cast 为目标类型指针（可变） */
    template<typename U> U* ptr() { return reinterpret_cast<U*>(&buf_); }

    /** @brief 将存储区 reinterpret_cast 为目标类型指针（常量） */
    template<typename U> const U* cptr() const { return reinterpret_cast<const U*>(&buf_); }

    /** @brief 根据当前状态析构 T 或 E */
    void destroy() {
        if (ok_) ptr<T>()->~T();
        else     ptr<E>()->~E();
    }

    /** @brief 存储区大小 = max(sizeof(T), sizeof(E)) */
    static constexpr size_t kSize  = sizeof(T) > sizeof(E) ? sizeof(T) : sizeof(E);

    /** @brief 存储区对齐 = max(alignof(T), alignof(E)) */
    static constexpr size_t kAlign = alignof(T) > alignof(E) ? alignof(T) : alignof(E);

    bool                          ok_;                  /**< true = 持有 T，false = 持有 E */
    alignas(kAlign) unsigned char buf_[kSize]{};        /**< 对齐存储区 */
};
