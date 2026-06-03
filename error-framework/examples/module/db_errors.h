/**
 * @file    db_errors.h
 * @brief   数据库模块自定义错误定义示例
 *
 * 演示模块如何使用 X-Macro 模式定义自己的错误码体系。
 * 模块仅需编写此头文件 + 引入框架 ef.h，无需修改框架核心代码。
 *
 * 使用方式：
 *   #include "ef/ef.h"
 *   #include "db_errors.h"
 *
 *   // 构造模块错误
 *   return EF_ERROR(db::errors::kConnectFail, "server %s:%d unreachable", host, port);
 *
 *   // 注册外部查表（可选，让框架 LogError 等识别模块错误）
 *   ef::SetExternalLookup(db::LookupError);
 *
 * @author  backend-common-components
 * @date    2026-06-03
 */

#pragma once

#include "../../include/ef/ef.h"

// ============================================================================
// 数据库模块错误定义
// ============================================================================
//
// 模块错误码号段: 10000 ~ 10099
//
// 使用 X-Macro 模式：
//   1. 定义错误列表宏 DB_ERRORS(X)
//   2. 调用 EF_GEN_ERROR_CODES 生成错误码常量
//   3. 调用 EF_GEN_ERROR_TABLE 生成元数据表和查表函数
// ============================================================================

// 第一步：定义错误列表（X-Macro 模式）
#define DB_ERRORS(X) \
    X(10001, ConnectFail,     "Database connection failed",              ef::Severity::Error) \
    X(10002, QueryTimeout,    "Query execution timeout",                 ef::Severity::Error) \
    X(10003, DupKey,          "Duplicate key violation in insert",       ef::Severity::Warn)  \
    X(10004, NotInitialized,  "Database not initialized, call Init() first", ef::Severity::Fatal) \
    X(10005, SchemaMismatch,  "Table schema does not match expected",    ef::Severity::Error) \
    X(10006, TransactionFail, "Transaction commit failed, rolled back",  ef::Severity::Error) \
    X(10007, Deadlock,        "Deadlock detected, retry operation",      ef::Severity::Warn)

// 第二步：生成错误码常量 -> db::errors::kConnectFail, db::errors::kQueryTimeout, ...
namespace db {
namespace errors {
    EF_GEN_ERROR_CODES(DB_ERRORS)
}  // namespace errors
}  // namespace db

// 第三步：生成元数据表 + 查表函数 -> db::kErrorTable[], db::LookupDbError()
namespace db {
    EF_GEN_ERROR_TABLE(DB_ERRORS)
}  // namespace db

// 清理 X-Macro 列表
#undef DB_ERRORS