/**
 * @file    error_utils.h
 * @brief   错误处理工具函数（日志输出等）
 *
 * 提供开箱即用的 LogError() 函数，支持统一格式的日志输出与错误链打印。
 *
 * 更高级的上报 / 监控功能可在业务层自行实现，通常不需要修改本文件。
 *
 * @author  backend-common-components
 * @date    2026-05-31
 * @version 1.0.0
 */

#pragma once

#include "error.h"
#include <cstdio>

// ============================================================================
// LogError — 统一格式错误日志输出
// ============================================================================

/**
 * @brief  将 Error 以统一格式输出到 stderr，并递归打印错误链
 *
 * @param  err  待输出的错误对象
 *
 * 输出格式：
 * @code
 *   [level] what (code) @ file:line | context_message
 *     caused by: lower_level_what
 * @endcode
 *
 * 示例输出：
 * @code
 *   [3] File not found (1001) @ main.cpp:30 | cannot open 'config.ini'
 *   [3] Network timeout (2001) @ client.cpp:88 | retry exhausted
 *     caused by: Connection refused
 * @endcode
 *
 * @note  输出目标固定为 stderr。如需输出到文件或 syslog，
 *        可参考本函数自行实现。
 */
inline void LogError(const Error& err) {
    std::fprintf(stderr, "[%d] %s (%d) @ %s:%d | %s\n",
        static_cast<int>(err.level()),
        err.what(),
        err.code(),
        err.location().file,
        err.location().line,
        err.message().c_str());

    if (auto* c = err.cause()) {
        std::fprintf(stderr, "  caused by: %s\n", c->what());
    }
}
