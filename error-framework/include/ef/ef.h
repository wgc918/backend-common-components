/**
 * @file    ef.h
 * @brief   框架汇总头文件 — 包含所有必需组件
 *
 * 业务模块只需 #include "ef/ef.h" 并使用 ef::errors 命名空间中的
 * 内置错误码即可开始使用。如需自定义模块错误，额外 #include 模块的
 * 错误定义头文件即可。
 *
 * @author  backend-common-components
 * @date    2026-06-03
 */

#pragma once

#include "source_location.h"
#include "error_kind.h"
#include "error.h"
#include "result.h"
#include "macros.h"
#include "utils.h"
#include "error_def.h"

/**
 * @namespace ef
 * @brief   错误处理框架的顶层命名空间
 *
 * 核心类型速查：
 *   - Error          运行时错误对象（SBO + 错误链）
 *   - Result<T, E>   成功值或错误二选一容器
 *   - Result<void>   void 特化，仅表示成功或错误
 *   - ErrorInfo      错误元数据（code + name + message + severity）
 *   - Severity       错误严重程度枚举
 *   - SourceLocation 源码位置（文件/行/函数）
 *
 * 核心宏速查：
 *   - EF_ERROR(code, fmt, ...)         构造错误并自动捕获源码位置
 *   - EF_TRY(expr)                     解包 Result，错误时自动传播
 *   - EF_CHECK(expr)                   检查 void Result，错误时提前返回
 *   - EF_RETURN_IF(cond, code, ...)    条件为真时返回错误
 *   - EF_EXPECT_OR(cond, code, ...)    条件为假时返回错误
 *   - EF_SOURCE_LOC()                  捕获当前源码位置
 *
 * 内置错误码（ef::errors::）：
 *   - kOk(0), kUnknown(1), kInvalidArg(2), kNotSupported(3)
 *   - kNotFound(100), kPermission(101), kAlreadyExists(102), kEOF(103)
 *   - kTimeout(200), kConnectionRefused(201), kConnectionReset(202)
 *   - kParseError(300), kInvalidFormat(301)
 *   - kOutOfMemory(400), kOverflow(401), kOutOfRange(402), kEmpty(403), kFull(404)
 *   - kInternal(500), kUnimplemented(501)
 *   - 10000+ 由业务模块自定义
 */