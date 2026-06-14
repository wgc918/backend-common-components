//-----------------------------------------------------------------------------
// 版权所有 (C) 2026 backend-common-components 项目
//
// 文件: error_kind.h
// 作者: wgc
// 创建日期: 2026年6月
// 最后修改: 2026年6月
//
// 描述:
//     定义整个错误框架的错误码体系。
//
// 功能特性:
//     - 错误等级枚举 ErrorLevel
//     - 错误信息结构体 ErrorInfo
//     - 内置错误码常量（errors 命名空间）
//     - 错误表查表函数 LookupInTable / LookupError
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
#include <cstddef>
#include <string>

namespace ef
{

/// @brief 错误等级枚举，表示错误的严重程度
enum class ErrorLevel : int
{
    Avoid,    ///< 可避免的错误
    CBC,      ///< 级联错误
    Warning,  ///< 警告
    Fatal     ///< 严重错误
};

/// @brief 将错误等级转换为字符串
/// @param level  错误等级枚举值
/// @return 错误等级的字符串描述
std::string ErrorLevel2string(ErrorLevel level);

/// @brief 错误信息结构体，存储错误码的元数据
struct ErrorInfo
{
    int         code;     ///< 错误码数值
    const char* name;     ///< 错误码名称
    const char* message;  ///< 错误描述信息
    ErrorLevel  level;    ///< 错误等级
};

/// @brief 内置错误码常量
namespace errors
{
constexpr int kUnknown       = 1;   ///< 未知错误
constexpr int kInvalidArg    = 2;   ///< 无效参数
constexpr int kNotSupported  = 3;   ///< 操作不支持
constexpr int kNotFound      = 4;   ///< 资源未找到
constexpr int kAlreadyExists = 5;   ///< 资源已存在
constexpr int kTimeout       = 6;   ///< 操作超时
constexpr int kInvalidFormat = 7;   ///< 无效格式
constexpr int kOverflow      = 8;   ///< 缓冲区溢出
constexpr int kOutOfRange    = 9;   ///< 索引超出范围
constexpr int kEmpty         = 10;  ///< 容器为空

}  // namespace errors

/// @brief 内置错误码信息表
inline constexpr ErrorInfo kBuiltinErrorTable[] = {
    {errors::kUnknown,       "Unknown",       "Unknown error",           ErrorLevel::Avoid  },
    {errors::kInvalidArg,    "InvalidArg",    "Invalid argument",        ErrorLevel::Avoid  },
    {errors::kNotSupported,  "NotSupported",  "Operation not supported", ErrorLevel::Warning},
    {errors::kNotFound,      "NotFound",      "Resource not found",      ErrorLevel::Warning},
    {errors::kAlreadyExists, "AlreadyExists", "Resource already exists", ErrorLevel::Warning},
    {errors::kTimeout,       "Timeout",       "Operation timed out",     ErrorLevel::Warning},
    {errors::kInvalidFormat, "InvalidFormat", "Invalid format",          ErrorLevel::Warning},
    {errors::kOverflow,      "Overflow",      "Buffer overflow",         ErrorLevel::Fatal  },
    {errors::kOutOfRange,    "OutOfRange",    "Index out of range",      ErrorLevel::Fatal  },
    {errors::kEmpty,         "Empty",         "Container is empty",      ErrorLevel::Warning},
};

/// @brief 在编译期错误表中按错误码查找 ErrorInfo
/// @tparam N      错误表大小（由编译器自动推导）
/// @param table   错误信息表引用
/// @param code    待查找的错误码
/// @return 指向 ErrorInfo 的指针，未找到则返回 nullptr
template <size_t N>
inline constexpr const ErrorInfo* LookupInTable(const ErrorInfo (&table)[N], int code) noexcept
{
    const ErrorInfo* ret = nullptr;
    for (size_t i = 0; i < N; i++)
    {
        if (table[i].code == code)
        {
            ret = &table[i];
            break;
        }
    }
    return ret;
}

/// @brief 统一错误码查找 — 先查内置表，再查外部注册表，兜底返回 Unknown
/// @param code  错误码
/// @return 指向 ErrorInfo 的指针，保证非空
inline const ErrorInfo* LookupError(int code) noexcept
{
    if (auto* info = LookupInTable(kBuiltinErrorTable, code))
    {
        return info;
    }
    extern const ErrorInfo* LookupExternalError(int code) noexcept;
    if (auto* info = LookupExternalError(code))
    {
        return info;
    }
    return LookupInTable(kBuiltinErrorTable, errors::kUnknown);
}

}  // namespace ef