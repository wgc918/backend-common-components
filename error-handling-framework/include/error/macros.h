/**
 * @file    macros.h
 * @brief   AEF 快捷构造错误宏
 *
 * AEF = "Abort / Error / Fail" 的缩写。
 * 一行代码即可构造 Error 对象并自动捕获 __FILE__ / __LINE__ / __FUNCTION__。
 *
 * @author  backend-common-components
 * @date    2026-05-31
 * @version 1.0.0
 */

#pragma once

#include "error.h"

// ============================================================================
// AEF — 快速构造错误宏
// ============================================================================

/**
 * @def    AEF(kind, fmt, ...)
 * @brief  快速构造 Error 对象，自动捕获源码位置
 *
 * @param  kind  ErrorKind 枚举值（不带 ErrorKind:: 前缀）
 * @param  fmt   printf 风格格式化字符串
 * @param  ...   格式化参数（可选）
 *
 * 使用示例：
 * @code
 *   if (!file.is_open())
 *       return AEF(FileNotFound, "cannot open '%s'", path.c_str());
 *
 *   if (port <= 0)
 *       return AEF(InvalidArg, "port must be positive, got %d", port);
 * @endcode
 *
 * @note  宏展开后等价于：
 *        Error(ErrorKind::kind, SOURCE_LOC(), fmt, ##__VA_ARGS__)
 */
#define AEF(kind, fmt, ...) \
    Error(ErrorKind::kind, SOURCE_LOC(), fmt, ##__VA_ARGS__)
