/**
 * @file    result.h
 * @brief   Result<T, E> — tagged union，成功值或错误二选一
 *
 * 核心设计：
 *   - 双模板参数：T = 成功类型，E = 错误类型（默认 Error）。
 *   - 底层为 aligned storage + placement new 实现的 tagged union。
 *   - 禁止拷贝（避免隐式数据重复），仅支持移动。
 *   - 提供 unwrap() / unwrap_err() / value_or() / operator-> 等解包方式。
 *
 * 使用示例：
 * @code
 *   Result<int> Div(int a, int b) {
 *       if (b == 0) return EF_ERROR(errors::kInvalidArg, "division by zero");
 *       return a / b;
 *   }
 *
 *   auto r = Div(10, 0);
 *   if (!r) { LogError(r.unwrap_err()); }
 *   else    { use(*r); }
 * @endcode
 *
 * @author  backend-common-components
 * @date    2026-06-03
 */

#pragma once

#include "error.h"
#include <new>
#include <type_traits>
#include <utility>

namespace ef {

// ============================================================================
// Result<T, E> — tagged union
// ============================================================================

/**
 * @class Result
 * @brief 成功值或错误二选一的容器
 *
 * @tparam T  成功时的值类型（可为 void）
 * @tparam E  错误时的值类型，默认为 Error
 *
 * 对于 T = void 的特化，Result 仅能表示成功或错误状态，不带返回值。
 */
template <typename T, typename E = Error>
class Result {
    // void 特化不允许持有值，由 Class template 处理时做 SFINAE。
    // 在此通用版本中 T 为具体类型。
public:
    // ========================================================================
    // 构造 / 析构 / 移动
    // ========================================================================

    /**
     * @brief 从成功值构造（const 左值引用 — 拷贝）
     */
    Result(const T& val) : ok_(true) {
        new (&buf_) T(val);
    }

    /**
     * @brief 从成功值构造（右值引用 — 移动，noexcept）
     */
    Result(T&& val) noexcept : ok_(true) {
        new (&buf_) T(std::move(val));
    }

    /**
     * @brief 从错误构造（const 左值 — 拷贝）
     */
    Result(const E& err) : ok_(false) {
        new (&buf_) E(err);
    }

    /**
     * @brief 从错误构造（右值 — 移动，noexcept）
     */
    Result(E&& err) noexcept : ok_(false) {
        new (&buf_) E(std::move(err));
    }

    /**
     * @brief 移动构造（noexcept）
     */
    Result(Result&& o) noexcept : ok_(o.ok_) {
        if (ok_) new (&buf_) T(std::move(*o.template ptr<T>()));
        else     new (&buf_) E(std::move(*o.template ptr<E>()));
    }

    /** @brief 析构，根据当前状态调用 T 或 E 的析构函数 */
    ~Result() { destroy(); }

    /** @brief 禁止拷贝构造（独占所有权语义） */
    Result(const Result&) = delete;

    /** @brief 禁止拷贝赋值 */
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
     * @brief  解包成功值（const 引用版本）
     *
     * @return const T& 成功值的引用
     *
     * @warning 若当前持有错误，调用 std::abort() 终止程序。
     */
    const T& unwrap() const {
        if (!ok_) std::abort();
        return *cptr<T>();
    }

    /**
     * @brief  解包成功值（可变引用版本）
     */
    T& unwrap() {
        if (!ok_) std::abort();
        return *ptr<T>();
    }

    /**
     * @brief  解包成功值（移动版本）
     */
    T unwrap_move() {
        if (!ok_) std::abort();
        return std::move(*ptr<T>());
    }

    /**
     * @brief  解包错误值（const 引用）
     *
     * @note 调用方需确保 is_err() 为 true。
     */
    const E& unwrap_err() const {
        return *cptr<E>();
    }

    /**
     * @brief  解包错误值（可变引用）
     */
    E& unwrap_err() {
        return *ptr<E>();
    }

    /**
     * @brief  错误时返回默认值（右值移动语义）
     *
     * @param  def  默认值
     * @return 成功时移动 T，错误时移动 def
     *
     * 使用示例：
     * @code
     *   int port = LoadConfig("server.json").value_or(8080);
     * @endcode
     */
    T value_or(T&& def) && {
        return ok_ ? std::move(*ptr<T>()) : std::forward<T>(def);
    }

    /**
     * @brief  错误时返回默认值（const 左值拷贝）
     */
    T value_or(const T& def) const & {
        return ok_ ? *cptr<T>() : def;
    }

    // ========================================================================
    // 便捷访问
    // ========================================================================

    /**
     * @brief  指针访问（-> / * 操作符），方便直接操作成功值
     *
     * @code
     *   auto r = GetConfig();
     *   if (r) { use(r->port); }  // 直接访问成员
     * @endcode
     *
     * @warning 若持有错误则 abort()。
     */
    T*       operator->()       { return ptr<T>(); }
    const T* operator->() const { return cptr<T>(); }

    T&       operator*()        { return unwrap(); }
    const T& operator*()  const { return unwrap(); }

    // ========================================================================
    // 错误转换（E -> Error 的隐式兼容）
    // ========================================================================

    /**
     * @brief  当 E == Error 时，可直接比较错误码
     *         用于 TRY/CHECK 宏的简洁判断
     */
    template <typename U = E,
              typename std::enable_if_t<std::is_same<U, Error>::value, int> = 0>
    bool is_error_code(int code) const noexcept {
        return is_err() && unwrap_err().code() == code;
    }

private:
    // ========================================================================
    // 内部辅助
    // ========================================================================

    template <typename U> U* ptr() {
        return reinterpret_cast<U*>(&buf_);
    }
    template <typename U> const U* cptr() const {
        return reinterpret_cast<const U*>(&buf_);
    }

    void destroy() {
        if (ok_) ptr<T>()->~T();
        else     ptr<E>()->~E();
    }

    static constexpr size_t kSize  = sizeof(T) > sizeof(E) ? sizeof(T) : sizeof(E);
    static constexpr size_t kAlign = alignof(T) > alignof(E) ? alignof(T) : alignof(E);

    bool                          ok_;             ///< true = 持有 T, false = 持有 E
    alignas(kAlign) unsigned char buf_[kSize]{};   ///< 对齐存储区
};

// ============================================================================
// Result<void, E> — void 特化（仅表示成功/错误，不持有值）
// ============================================================================

/**
 * @brief Result<void, E> 特化：仅表示成功或错误状态，不带返回值
 *
 * 用法：
 * @code
 *   Result<void> Init() {
 *       if (!config.Load()) return EF_ERROR(errors::kNotFound, "config missing");
 *       return {};  // 成功，无返回值
 *   }
 * @endcode
 */
template <typename E>
class Result<void, E> {
public:
    Result() : ok_(true) {}
    Result(const E& err) : ok_(false) { new (&buf_) E(err); }
    Result(E&& err) noexcept : ok_(false) { new (&buf_) E(std::move(err)); }

    Result(Result&& o) noexcept : ok_(o.ok_) {
        if (!ok_) new (&buf_) E(std::move(*o.template ptr<E>()));
    }

    ~Result() { destroy(); }

    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;

    bool is_ok()  const noexcept { return ok_; }
    bool is_err() const noexcept { return !ok_; }
    explicit operator bool() const noexcept { return ok_; }

    const E& unwrap_err() const { return *cptr<E>(); }
    E&       unwrap_err()       { return *ptr<E>(); }

private:
    template <typename U> U* ptr()       { return reinterpret_cast<U*>(&buf_); }
    template <typename U> const U* cptr() const { return reinterpret_cast<const U*>(&buf_); }

    void destroy() {
        if (!ok_) ptr<E>()->~E();
    }

    bool                          ok_;
    alignas(alignof(E)) unsigned char buf_[sizeof(E)]{};
};

}  // namespace ef