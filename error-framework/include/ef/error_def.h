/**
 * @file    error_def.h
 * @brief   模块自定义错误定义宏
 *
 * 提供宏帮助业务模块快速定义自己的错误码和查表函数。
 * 模块只需在自定义头文件中使用这些宏，无需修改框架代码。
 *
 * 使用方式（推荐 X-Macro 模式）：
 * @code
 *   // db_errors.h
 *   #pragma once
 *   #include "ef/ef.h"
 *
 *   // 第一步：定义错误列表（X-Macro）
 *   #define DB_ERRORS(X) \
 *       X(10001, ConnectFail,    "Database connection failed",           ef::Severity::Error) \
 *       X(10002, QueryTimeout,   "Query execution timeout",              ef::Severity::Error) \
 *       X(10003, DupKey,         "Duplicate key violation",              ef::Severity::Warn)
 *
 *   // 第二步：生成错误码常量
 *   namespace db::errors {
 *       EF_GEN_ERROR_CODES(DB_ERRORS)
 *   }
 *
 *   // 第三步：生成元数据表和查表函数
 *   namespace db {
 *       EF_GEN_ERROR_TABLE(DB_ERRORS)
 *   }
 *
 *   #undef DB_ERRORS
 * @endcode
 *
 * 使用方式 2（手动定义，最简单）：
 * @code
 *   namespace db::errors {
 *       constexpr int kConnectFail  = 10001;
 *       constexpr int kQueryTimeout = 10002;
 *   }
 *   // 使用时：EF_ERROR(db::errors::kConnectFail, "server %s", addr);
 * @endcode
 *
 * @author  backend-common-components
 * @date    2026-06-03
 */

#pragma once

#include "error_kind.h"

/**
 * @def    EF_GEN_ERROR_CODES(XMacroList)
 * @brief  从 X-Macro 错误列表生成 constexpr int 错误码常量
 *
 * @param  XMacroList  一个 X-Macro 列表，如 DB_ERRORS(X)
 *
 * 生成的代码示例：
 * @code
 *   constexpr int kConnectFail = 10001;
 *   constexpr int kQueryTimeout = 10002;
 * @endcode
 */
#define EF_GEN_ERROR_CODES(XMacroList)                    \
    XMacroList(EF_GEN_CODE)

#define EF_GEN_CODE(code, name, msg, sev)                 \
    constexpr int k##name = code;

/**
 * @def    EF_GEN_ERROR_TABLE(XMacroList)
 * @brief  从 X-Macro 错误列表生成错误元数据表和查表函数
 *
 * @param  XMacroList  一个 X-Macro 列表，如 DB_ERRORS(X)
 *
 * 生成的代码：
 *   - inline constexpr ef::ErrorInfo kErrorTable[] = { ... };
 *   - inline const ef::ErrorInfo* LookupError(int code) noexcept { ... }
 */
#define EF_GEN_ERROR_TABLE(XMacroList)                    \
    inline constexpr ::ef::ErrorInfo kErrorTable[] = {    \
        XMacroList(EF_GEN_ENTRY)                          \
    };                                                    \
    inline const ::ef::ErrorInfo* LookupError(int code) noexcept { \
        return ::ef::LookupInTable(kErrorTable, code);    \
    }

#define EF_GEN_ENTRY(code, name, msg, sev)                \
    { code, #name, msg, sev },
