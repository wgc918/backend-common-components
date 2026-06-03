/**
 * @file    source_location.h
 * @brief   源码位置信息 — C++17 兼容实现
 *
 * C++20 引入了 std::source_location，本框架目标为 C++17，
 * 因此通过 SOURCE_LOC() 宏 + SourceLocation 结构体实现等价功能。
 *
 * @author  backend-common-components
 * @date    2026-06-03
 */

#pragma once

namespace ef {

/**
 * @brief 记录源码位置（文件、行号、函数名）
 *
 * 通常不需要手动构造，由 SOURCE_LOC() 宏自动填充。
 */
struct SourceLocation {
    const char* file;      ///< 源码文件名（__FILE__）
    int         line;      ///< 源码行号（__LINE__）
    const char* function;  ///< 函数名（__FUNCTION__）
};

}  // namespace ef

/**
 * @brief 自动捕获当前源码位置的便捷宏
 *
 * @note 必须在函数体内调用（__FUNCTION__ 要求）。
 *
 * 示例：
 * @code
 *   auto loc = EF_SOURCE_LOC();
 *   // loc.file == "main.cpp", loc.line == 42
 * @endcode
 */
#define EF_SOURCE_LOC() ::ef::SourceLocation{ __FILE__, __LINE__, __FUNCTION__ }