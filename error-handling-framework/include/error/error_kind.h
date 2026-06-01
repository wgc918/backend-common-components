/**
 * @file    error_kind.h
 * @brief   错误类型枚举定义与查表机制
 *
 * 本文件定义了框架内置的 ErrorKind 枚举、ErrorLevel 等级枚举、
 * ErrorEntry 元数据结构以及编译期查表函数 LookupError()。
 *
 * 扩展方式：
 *   1. 在 ErrorKind 枚举中添加新值
 *   2. 在 kErrorTable 中添加对应的 ErrorEntry
 *
 * @author  backend-common-components
 * @date    2026-05-31
 * @version 1.0.0
 */

#pragma once

// ============================================================================
// ErrorKind — 错误类型枚举
// ============================================================================

/** @brief 错误类型枚举，每个值对应一种预定义的错误场景 */
enum class ErrorKind : int {
    None = 0,          /**< 无错误，一切正常                   */
    Unknown,           /**< 未知错误                           */
    InvalidArg,        /**< 无效参数                           */
    FileNotFound,      /**< 文件未找到                         */
    FilePermission,    /**< 文件权限不足                       */
    NetworkTimeout,    /**< 网络超时                           */
    ConnectionRefused, /**< 连接被拒绝                         */
    ParseError,        /**< 解析错误（JSON / YAML / INI 等）   */
    InvalidFormat,     /**< 格式非法                           */
    // ===== 业务扩展从此处添加 =====
};

// ============================================================================
// ErrorLevel — 错误严重程度
// ============================================================================

/** @brief 错误严重程度等级，从 Debug 到 Fatal 递增 */
enum class ErrorLevel : int {
    Debug = 0,  /**< 调试信息，不构成错误               */
    Info  = 1,  /**< 一般信息，记录即可                 */
    Warn  = 2,  /**< 警告，不影响主流程但需要关注       */
    Error = 3,  /**< 错误，当前操作失败                 */
    Fatal = 4,  /**< 致命错误，程序可能无法继续         */
};

// ============================================================================
// ErrorEntry — 错误条目元数据
// ============================================================================

/**
 * @brief 错误条目元数据，将 ErrorKind 与错误码、描述、等级绑定
 *
 * 所有条目存储在编译期常量数组 kErrorTable 中，
 * LookupError() 通过线性查找返回对应条目的指针。
 */
struct ErrorEntry {
    ErrorKind   kind;     /**< 错误类型枚举值                 */
    int         code;     /**< 错误码（整型，便于上报 / 比对） */
    const char* message;  /**< 错误描述字符串（字面量）        */
    ErrorLevel  level;    /**< 错误严重程度                    */
};

// ============================================================================
// kErrorTable — 全局错误查表（编译期常量）
// ============================================================================

/**
 * @brief 全局错误查表，编译期常量数组
 *
 * 每种 ErrorKind 对应一条记录。添加新错误类型时在此处追加一行即可。
 *
 * @note  错误码分段约定（非强制）：
 *        -     0 ~   999 : 通用错误
 *        -  1000 ~  1999 : IO 类错误
 *        -  2000 ~  2999 : 网络类错误
 *        -  3000 ~  3999 : 解析类错误
 *        - 10000 以上     : 业务自定义
 */
inline constexpr ErrorEntry kErrorTable[] = {
    { ErrorKind::None,              0,    "No error",              ErrorLevel::Debug },
    { ErrorKind::Unknown,           1,    "Unknown error",         ErrorLevel::Error },
    { ErrorKind::InvalidArg,        2,    "Invalid argument",      ErrorLevel::Warn  },
    { ErrorKind::FileNotFound,      1001, "File not found",        ErrorLevel::Error },
    { ErrorKind::FilePermission,    1002, "Permission denied",     ErrorLevel::Error },
    { ErrorKind::NetworkTimeout,    2001, "Network timeout",       ErrorLevel::Error },
    { ErrorKind::ConnectionRefused, 2002, "Connection refused",    ErrorLevel::Error },
    { ErrorKind::ParseError,        3001, "Parse error",           ErrorLevel::Warn  },
    { ErrorKind::InvalidFormat,     3002, "Invalid format",        ErrorLevel::Warn  },
};

// ============================================================================
// LookupError — 运行时查表函数
// ============================================================================

/**
 * @brief  根据 ErrorKind 查找对应的 ErrorEntry
 *
 * 在编译期常量数组 kErrorTable 中线性查找。
 * 若传入的 ErrorKind 未注册，则回退到 kErrorTable[0]（None）。
 *
 * @param  k  待查找的错误类型
 * @return    指向对应 ErrorEntry 的常量指针，保证非空
 *
 * @note  复杂度 O(n)，n = 数组长度。对于框架内置的 ~10 个条目，线性查找无性能问题。
 *        若业务扩展到数百个条目，可考虑用 switch-case 或编译期 hash 替代。
 */
inline constexpr const ErrorEntry* LookupError(ErrorKind k) noexcept {
    for (const auto& e : kErrorTable) {
        if (e.kind == k) return &e;
    }
    return &kErrorTable[0];
}
