/*
 * =============================================================================
 * Module:        simple_logger
 * Description:   轻量级日志宏，用于线程池实现中的调试和错误日志输出。
 *                不依赖任何外部日志模块，保证实现的独立性。
 * =============================================================================
 */

#pragma once

#include <iostream>
#include <string>
#include <ctime>
#include <sstream>

#ifdef ENABLE_LOGGING

inline void log_debug(const std::string &msg) {
    auto now = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    std::cout << "[DEBUG][" << buf << "] " << msg << std::endl;
}

inline void log_error(const std::string &msg) {
    auto now = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    std::cerr << "[ERROR][" << buf << "] " << msg << std::endl;
}

#define LOG_DEBUG(msg) log_debug(msg)
#define LOG_ERROR(msg) log_error(msg)

#else

#define LOG_DEBUG(msg) ((void)0)
#define LOG_ERROR(msg) ((void)0)

#endif