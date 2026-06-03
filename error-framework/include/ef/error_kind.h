/**
 * @file    error_kind.h
 * @brief   错误码体系 — Severity、ErrorInfo、内置错误码 + 查表
 *
 * 设计要点：
 *   - 使用 int 作为错误标识，替代 enum class，便于模块扩展。
 *   - 错误码分段管理：0~999 框架内置，10000+ 业务模块自定义。
 *   - 所有错误元数据通过编译期常量表 + constexpr 查表函数获取。
 *
 * 扩展方式：
 *   模块在自定义头文件中定义 constexpr int 错误码常量，
 *   并可选地通过 EF_REGISTER_ERROR_INFO 宏注册元数据。
 *
 * @author  backend-common-components
 * @date    2026-06-03
 */

#pragma once

#include <cstddef>

namespace ef {

// ============================================================================
// Severity — 错误严重程度
// ============================================================================

/**
 * @brief 错误严重程度，从低到高排列
 *
 * 推荐使用场景：
 *   - Debug: 调试期输出，不构成真正的错误
 *   - Info:  运行时提示信息
 *   - Warn:  警告，操作可继续但需关注
 *   - Error: 错误，当前操作失败
 *   - Fatal: 致命错误，程序可能无法继续
 */
enum class Severity : int {
    Debug = 0,
    Info  = 1,
    Warn  = 2,
    Error = 3,
    Fatal = 4,
};

// ============================================================================
// ErrorInfo — 错误元数据
// ============================================================================

/**
 * @brief 错误条目元数据，将错误码与名称、描述、严重程度绑定
 */
struct ErrorInfo {
    int         code;       ///< 错误码
    const char* name;       ///< 错误名称（如 "InvalidArg"）
    const char* message;    ///< 人类可读描述
    Severity    severity;   ///< 严重程度
};

// ============================================================================
// 内置错误码 (0 ~ 999)
// ============================================================================

/**
 * @brief 框架内置错误码常量
 *
 * 错误码分段约定：
 *   -    0       : 成功（不是错误）
 *   -    1 ~  99 : 通用逻辑错误（参数、状态等）
 *   -  100 ~ 199 : IO 类错误
 *   -  200 ~ 299 : 网络类错误
 *   -  300 ~ 399 : 解析/格式类错误
 *   -  400 ~ 499 : 资源类错误
 *   -  500 ~ 999 : 保留扩展
 *   -  10000+    : 业务模块自定义（每个模块建议分配 100 个号段）
 */
namespace errors {
    // 成功（非错误）
    constexpr int kOk              = 0;

    // 通用 (1~99)
    constexpr int kUnknown         = 1;
    constexpr int kInvalidArg      = 2;
    constexpr int kNotSupported    = 3;

    // IO (100~199)
    constexpr int kNotFound        = 100;
    constexpr int kPermission      = 101;
    constexpr int kAlreadyExists   = 102;
    constexpr int kEOF             = 103;

    // 网络 (200~299)
    constexpr int kTimeout         = 200;
    constexpr int kConnectionRefused = 201;
    constexpr int kConnectionReset = 202;

    // 解析 (300~399)
    constexpr int kParseError      = 300;
    constexpr int kInvalidFormat   = 301;

    // 资源 (400~499)
    constexpr int kOutOfMemory     = 400;
    constexpr int kOverflow        = 401;
    constexpr int kOutOfRange      = 402;
    constexpr int kEmpty           = 403;
    constexpr int kFull            = 404;

    // 内部错误 (500~599)
    constexpr int kInternal        = 500;
    constexpr int kUnimplemented   = 501;

}  // namespace errors

// ============================================================================
// 内置错误元数据表
// ============================================================================

/**
 * @brief 框架内置错误元数据表（编译期常量）
 *
 * 每种错误码对应一条 ErrorInfo。添加新内置错误时在此追加即可。
 */
inline constexpr ErrorInfo kBuiltinErrorTable[] = {
    { errors::kOk,               "Ok",                "Success",                        Severity::Debug },
    { errors::kUnknown,          "Unknown",           "Unknown error",                  Severity::Error },
    { errors::kInvalidArg,       "InvalidArg",        "Invalid argument",               Severity::Warn  },
    { errors::kNotSupported,     "NotSupported",      "Operation not supported",        Severity::Error },
    { errors::kNotFound,         "NotFound",          "Resource not found",             Severity::Error },
    { errors::kPermission,       "Permission",        "Permission denied",              Severity::Error },
    { errors::kAlreadyExists,    "AlreadyExists",     "Resource already exists",        Severity::Warn  },
    { errors::kEOF,              "EOF",               "Unexpected end of file",         Severity::Error },
    { errors::kTimeout,          "Timeout",           "Operation timed out",            Severity::Error },
    { errors::kConnectionRefused,"ConnectionRefused", "Connection refused",             Severity::Error },
    { errors::kConnectionReset,  "ConnectionReset",   "Connection reset by peer",       Severity::Error },
    { errors::kParseError,       "ParseError",        "Parse error",                    Severity::Warn  },
    { errors::kInvalidFormat,    "InvalidFormat",     "Invalid format",                 Severity::Warn  },
    { errors::kOutOfMemory,      "OutOfMemory",       "Out of memory",                  Severity::Fatal },
    { errors::kOverflow,         "Overflow",          "Buffer overflow",                Severity::Error },
    { errors::kOutOfRange,       "OutOfRange",        "Index out of range",             Severity::Error },
    { errors::kEmpty,            "Empty",             "Container is empty",             Severity::Error },
    { errors::kFull,             "Full",              "Container is full",              Severity::Error },
    { errors::kInternal,         "Internal",          "Internal error",                 Severity::Fatal },
    { errors::kUnimplemented,    "Unimplemented",     "Function not implemented",       Severity::Error },
};

// ============================================================================
// LookupError — 查表函数
// ============================================================================

/**
 * @brief  在线性表中查找错误元数据
 * @tparam  N  表的大小（编译器推导）
 * @param   table  错误元数据表
 * @param   code   要查找的错误码
 * @return  指向匹配的 ErrorInfo 的指针，未找到返回 nullptr
 */
template <size_t N>
inline constexpr const ErrorInfo* LookupInTable(const ErrorInfo (&table)[N], int code) noexcept {
    for (size_t i = 0; i < N; ++i) {
        if (table[i].code == code) return &table[i];
    }
    return nullptr;
}

/**
 * @brief  查找错误元数据（先查内置表，后查外部注册表）
 * @param  code  错误码
 * @return 指向 ErrorInfo 的指针，保证非空（未找到返回 Unknown 的条目）
 *
 * @note  复杂度 O(n)，对于当前规模已足够。
 *        如需支持海量错误码，可升级为二分查找或编译期 hash。
 */
inline const ErrorInfo* LookupError(int code) noexcept {
    // 1. 先查内置表
    if (auto* info = LookupInTable(kBuiltinErrorTable, code)) {
        return info;
    }
    // 2. 再查外部注册表（如果模块注册了的话，见 error.h 中的 ExternalRegistry）
    extern const ErrorInfo* LookupExternalError(int code) noexcept;
    if (auto* info = LookupExternalError(code)) {
        return info;
    }
    // 3. 回退到 Unknown
    return LookupInTable(kBuiltinErrorTable, errors::kUnknown);
}

}  // namespace ef