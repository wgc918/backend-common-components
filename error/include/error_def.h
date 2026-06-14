//-----------------------------------------------------------------------------
// 版权所有 (C) 2026 backend-common-components 项目
//
// 文件: error_def.h
// 作者: wgc
// 创建日期: 2026年6月
// 最后修改: 2026年6月
//
// 描述:
//     提供宏帮助业务模块快速定义自己的错误码和查表函数。
//     模块只需在自定义头文件中使用这些宏，无需修改框架代码。
//
// 功能特性:
//     - EF_GEN_ERROR_CODES  从 X-Macro 列表生成错误码常量
//     - EF_GEN_ERROR_META   从 X-Macro 列表生成错误元数据表与查表函数
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

/// @brief 辅助宏：连接两个宏参数
#define EF_CONCAT(a, b) a##b

/// @brief 从 X-Macro 错误列表生成 constexpr int 错误码常量
/// @param XMacroList  一个 X-Macro 列表，如 DB_ERRORS(X)
#define EF_GEN_ERROR_CODES(XMacroList) XMacroList(EF_GEN_CODE)

/// @brief X-Macro 辅助：展开为 constexpr int 错误码
#define EF_GEN_CODE(code, name, msg, level) constexpr int k##name = code;

/// @brief 从 X-Macro 列表生成错误信息表与查表函数
/// @param XMacroList  一个 X-Macro 列表，如 DB_ERRORS(X)
#define EF_GEN_ERROR_META(XMacroList)                                            \
    inline constexpr ::ef::ErrorInfo kErrorTable[] = {XMacroList(EF_GEN_ENTRY)}; \
    namespace EF_CONCAT(m_, XMacroList)                                          \
    {                                                                            \
    XMacroList(EF_GEN_CODE)                                                      \
    }                                                                            \
    inline const ::ef::ErrorInfo* LookupError(int code) noexcept                 \
    {                                                                            \
        return ::ef::LookupInTable(kErrorTable, code);                           \
    }

/// @brief X-Macro 辅助：展开为 ErrorInfo 表项
#define EF_GEN_ENTRY(code, name, msg, level) {code, #name, msg, level},