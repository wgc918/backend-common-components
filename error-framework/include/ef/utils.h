/**
 * @file    utils.h
 * @brief   错误处理工具函数 — 日志输出、格式化等
 *
 * 提供开箱即用的 LogError() 等函数。业务模块可参考实现自定义的
 * 上报 / 监控逻辑。
 *
 * @author  backend-common-components
 * @date    2026-06-03
 */

#pragma once

#include "error.h"
#include <cstdio>
#include <cstdlib>

namespace ef {

// 前置声明（用于 LogError 中调用）
inline void print_cause_chain(const Error& err, int depth);

// ============================================================================
// LogError — 统一格式错误日志输出
// ============================================================================

/**
 * @brief  将 Error 以统一格式输出到 stderr，并递归打印错误链
 *
 * @param  err     待输出的错误对象
 * @param  prefix  可选前缀（如 "ERROR: "）
 *
 * 输出格式（无错误链）：
 * @code
 *   [Warn] NotFound (100) @ main.cpp:30 | cannot open 'config.ini'
 * @endcode
 *
 * 输出格式（有错误链）：
 * @code
 *   [Error] Timeout (200) @ client.cpp:88 | request failed after 3 retries
 *      -> caused by: ConnectionRefused (201) @ net.cpp:44 | connect to 10.0.0.1:8080
 * @endcode
 */
inline void LogError(const Error& err, const char* prefix = "") {
    auto* info = LookupError(err.code());

    // 严重程度标签
    const char* sev_str = "?";
    switch (info->severity) {
        case Severity::Debug: sev_str = "Debug"; break;
        case Severity::Info:  sev_str = "Info";  break;
        case Severity::Warn:  sev_str = "Warn";  break;
        case Severity::Error: sev_str = "Error"; break;
        case Severity::Fatal: sev_str = "FATAL"; break;
    }

    std::fprintf(stderr, "%s[%s] %s (%d) @ %s:%d",
        prefix,
        sev_str,
        info->name,
        err.code(),
        err.location().file,
        err.location().line);

    if (err.has_message()) {
        std::fprintf(stderr, " | %s", err.message().c_str());
    }
    std::fprintf(stderr, "\n");

    // 递归打印错误链
    if (err.has_cause()) {
        print_cause_chain(*err.cause(), 1);
    }
}

/**
 * @brief 仅返回格式化的字符串（不输出到 stderr）
 *
 * @param err  错误对象
 * @return 完整格式化字符串（含错误链）
 */
inline std::string ErrorString(const Error& err) {
    return err.to_string();
}

// ============================================================================
// Panic — 致命错误终止
// ============================================================================

/**
 * @brief  遇到致命错误时输出日志并终止程序
 *
 * @param  err  致命错误对象
 *
 * @note   调用 LogError() 后调用 std::abort()。
 */
[[noreturn]] inline void Panic(const Error& err) {
    LogError(err, "PANIC: ");
    std::abort();
}

// ============================================================================
// 内部辅助
// ============================================================================

/** @brief 递归打印错误链 */
inline void print_cause_chain(const Error& err, int depth) {
    auto* info = LookupError(err.code());
    for (int i = 0; i < depth; ++i) {
        std::fprintf(stderr, "  ");
    }
    std::fprintf(stderr, "-> caused by: %s (%d) @ %s:%d",
        info->name,
        err.code(),
        err.location().file,
        err.location().line);

    if (err.has_message()) {
        std::fprintf(stderr, " | %s", err.message().c_str());
    }
    std::fprintf(stderr, "\n");

    if (err.has_cause()) {
        print_cause_chain(*err.cause(), depth + 1);
    }
}

}  // namespace ef