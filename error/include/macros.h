//-----------------------------------------------------------------------------
// 版权所有 (C) 2026 backend-common-components 项目
//
// 文件: macros.h
// 作者: wgc
// 创建日期: 2026年6月
// 最后修改: 2026年6月
//
// 描述:
//     提供错误处理框架的便捷宏，简化错误创建、传播和检查的样板代码。
//
// 功能特性:
//     - EF_ERROR / EF_MAKE  创建带/不带上下文信息的 Error 对象
//     - EF_TRY              传播错误 — 若结果为 Err 则提前返回
//     - EF_CHECK            检查结果 — 若结果为 Err 则提前返回
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
#include "error.h"

/// @brief 创建带上下文信息的 Error 对象
/// @param code  错误码
/// @param fmt   格式化字符串
/// @param ...   格式化参数
#define EF_ERROR(code, fmt, ...) ::ef::Error(code, EF_SOURCE_LOC(), fmt, ##__VA_ARGS__)

/// @brief 创建不带上下文信息的 Error 对象
/// @param code  错误码
#define EF_MAKE(code) ::ef::Error(code, EF_SOURCE_LOC())

/// @brief 传播错误 — 若 expr 结果为 Err 则提前返回该错误
/// @param expr  返回 Result 的表达式
#define EF_TRY(expr)                    \
    ({                                  \
        auto result = (expr);           \
        if (!result)                    \
            return result.unwrap_err(); \
        result.unwrap_move();           \
    })

/// @brief 检查结果 — 若 expr 结果为 Err 则提前返回该错误
/// @param expr  返回 Result 的表达式
#define EF_CHECK(expr)                 \
    do                                 \
    {                                  \
        auto result = (expr);          \
        if (!result)                   \
            return result.unwrap_err() \
    } while (false)