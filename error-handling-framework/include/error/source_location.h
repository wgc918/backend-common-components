/**
 * @file    source_location.h
 * @brief   C++17 兼容的源码位置信息结构体
 *
 * C++20 引入了 std::source_location，但本框架目标为 C++17。
 * 因此提供 SourceLocation 结构体 + SOURCE_LOC() 宏作为替代方案，
 * 由 __FILE__、__LINE__、__FUNCTION__ 预定义宏自动填充。
 *
 * @author  backend-common-components
 * @date    2026-05-31
 * @version 1.0.0
 */

#pragma once

// ============================================================================
// SourceLocation — 源码位置
// ============================================================================

/**
 * @brief 记录源码位置信息（文件、行号、函数名）
 *
 * 由 SOURCE_LOC() 宏自动填充，通常在 AEF() 宏中隐式使用，
 * 用户无需手动构造。
 */
struct SourceLocation {
    const char* file;      /**< 源码文件名（__FILE__）         */
    int         line;      /**< 源码行号（__LINE__）           */
    const char* function;  /**< 函数名（__FUNCTION__）         */
};

// ============================================================================
// SOURCE_LOC — 便捷宏
// ============================================================================

/**
 * @brief  自动捕获当前源码位置
 *
 * 使用示例：
 * @code
 *   auto loc = SOURCE_LOC();
 *   loc.file == "main.cpp", loc.line == 42, loc.function == "main"
 * @endcode
 *
 * @note 必须在函数体内使用，不能在全局作用域中调用（__FUNCTION__ 不可用）。
 */
#define SOURCE_LOC() SourceLocation{ __FILE__, __LINE__, __FUNCTION__ }
