/**
 * @file    macros.h
 * @brief   便捷宏 — 简化错误的构造、传播和处理
 *
 * 本文件提供：
 *   - EF_ERROR:   快速构造 Error 对象
 *   - EF_MAKE:    更语义化的 EF_ERROR 别名
 *   - EF_TRY:     自动错误传播（Rust ? 操作符风格）
 *   - EF_CHECK:   检查 Result 状态并提前返回
 *
 * @author  backend-common-components
 * @date    2026-06-03
 */

#pragma once

#include "error.h"

// ============================================================================
// EF_ERROR — 快速构造错误宏
// ============================================================================

/**
 * @def    EF_ERROR(code, fmt, ...)
 * @brief  快速构造 Error 对象，自动捕获源码位置
 *
 * @param  code  错误码（框架内置或模块自定义）
 * @param  fmt   printf 风格格式化字符串
 * @param  ...   格式化参数（可选）
 *
 * 使用示例：
 * @code
 *   if (!file.is_open())
 *       return EF_ERROR(errors::kNotFound, "cannot open '%s'", path);
 *
 *   if (port <= 0)
 *       return EF_ERROR(errors::kInvalidArg, "port must be positive, got %d", port);
 * @endcode
 */
#define EF_ERROR(code, fmt, ...) \
    ::ef::Error(code, EF_SOURCE_LOC(), fmt, ##__VA_ARGS__)

/**
 * @def    EF_MAKE(code, fmt, ...)
 * @brief  EF_ERROR 的语义化别名
 */
#define EF_MAKE(code, fmt, ...) EF_ERROR(code, fmt, ##__VA_ARGS__)

// ============================================================================
// EF_TRY — 错误传播宏（Rust ? 操作符风格）
// ============================================================================

/**
 * @def    EF_TRY(expr)
 * @brief  尝试解包 Result：成功则返回值，失败则向外传播错误
 *
 * 使用场景：在返回 Result<T> 的函数中调用另一个返回 Result<U> 的函数，
 * 如果被调函数返回错误，则自动向上传播。
 *
 * @param  expr  返回 Result<T> 的表达式
 *
 * 展开效果：
 *   auto _ef_tmp = (expr);
 *   if (!_ef_tmp) return _ef_tmp.unwrap_err();
 *
 * 使用示例：
 * @code
 *   Result<int> process() {
 *       auto value = EF_TRY(compute_value());  // 失败时自动 return
 *       auto more  = EF_TRY(compute_more());   // 失败时自动 return
 *       return value + more;
 *   }
 * @endcode
 *
 * @note  EF_TRY 要求表达式的错误类型与当前函数返回的错误类型兼容。
 *        如果 E 不相同，可先用 EF_TRY_AS 进行类型转换。
 */
#define EF_TRY(expr)                                        \
    ({                                                      \
        auto _ef_try_result = (expr);                       \
        if (!_ef_try_result) {                              \
            return _ef_try_result.unwrap_err();             \
        }                                                   \
        _ef_try_result.unwrap_move();                       \
    })

// ============================================================================
// EF_CHECK — 检查 Result 状态宏
// ============================================================================

/**
 * @def    EF_CHECK(expr)
 * @brief  检查 Result<void> 是否成功，失败则提前返回
 *
 * 适用于 void 返回值的函数（或仅检查副作用，不需要取值）。
 *
 * @param  expr  返回 Result<void> 的表达式
 *
 * 使用示例：
 * @code
 *   Result<void> Init() {
 *       EF_CHECK(load_config());
 *       EF_CHECK(connect_db());
 *       EF_CHECK(start_server());
 *       return {};
 *   }
 * @endcode
 */
#define EF_CHECK(expr)                                      \
    do {                                                    \
        auto _ef_check_result = (expr);                     \
        if (!_ef_check_result) {                            \
            return _ef_check_result.unwrap_err();           \
        }                                                   \
    } while (false)

// ============================================================================
// EF_RETURN_IF — 条件错误返回
// ============================================================================

/**
 * @def    EF_RETURN_IF(cond, code, fmt, ...)
 * @brief  条件为真时构造错误并返回
 *
 * @param  cond  布尔条件表达式
 * @param  code  错误码
 * @param  fmt   格式化字符串
 * @param  ...   格式化参数
 *
 * 使用示例：
 * @code
 *   Result<void> SetPort(int port) {
 *       EF_RETURN_IF(port <= 0, errors::kInvalidArg, "port must be > 0, got %d", port);
 *       EF_RETURN_IF(port > 65535, errors::kInvalidArg, "port too large: %d", port);
 *       // ... 实际逻辑
 *       return {};
 *   }
 * @endcode
 */
#define EF_RETURN_IF(cond, code, fmt, ...)                  \
    do {                                                    \
        if (cond) {                                         \
            return EF_ERROR(code, fmt, ##__VA_ARGS__);      \
        }                                                   \
    } while (false)

// ============================================================================
// EF_EXPECT_OR — 断言风格错误返回
// ============================================================================

/**
 * @def    EF_EXPECT_OR(cond, code, fmt, ...)
 * @brief  条件为假时构造错误并返回（"期望条件成立，否则报错"）
 *
 * @param  cond  期望为真的条件
 * @param  code  错误码
 * @param  fmt   格式化字符串
 * @param  ...   格式化参数
 *
 * 使用示例：
 * @code
 *   Result<std::string> ReadLine(File& f) {
 *       EF_EXPECT_OR(f.is_open(), errors::kNotFound, "file not open");
 *       std::string line;
 *       // ...
 *       return line;
 *   }
 * @endcode
 */
#define EF_EXPECT_OR(cond, code, fmt, ...) do { if (!(cond)) { return EF_ERROR(code, fmt, ##__VA_ARGS__); } } while (false)

// ============================================================================
// 模块自定义错误辅助宏（引用 error_def.h 中的定义）
// ============================================================================