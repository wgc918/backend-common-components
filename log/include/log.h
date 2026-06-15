/*
 * =============================================================================
 * Project:       backend-common-components
 * Module:        logger
 * Author:        GuochengWu
 * Email:         3524515056@qq.com
 *
 * Copyright (c) 2026 GuochengWu. All rights reserved.
 *
 * Description:
 *   高性能、可配置的日志管理组件，是系统运维与问题排查的核心支撑。
 *
 *   核心特性：
 *     1. 单例模式全局唯一实例，保证日志输出的原子性与一致性；
 *     2. 五级日志级别 (DEBUG/INFO/WARNING/ERROR/FATAL)，支持动态切换；
 *     3. 基于 snprintf 栈缓冲区的高性能格式化，避免堆分配；
 *     4. 内部缓冲 + 批量刷新策略，大幅减少系统调用；
 *     5. 毫秒级时间戳精度，线程安全的时间格式化；
 *     6. 日志文件自动轮转 (rotate)，防止单文件过大；
 *     7. 控制台 / 文件双输出，可独立开关。
 *
 *   性能优化要点 (v2.0):
 *     - 以 snprintf + 栈缓冲区(4KB) 替代 std::stringstream，消除堆分配与虚函数开销
 *     - 批量缓冲刷新策略：DEBUG/INFO 级别仅在缓冲区满或遇到 WARNING 时刷新
 *     - localtime_r 替代 localtime，保证线程安全
 *     - std::lock_guard 替代裸 lock/unlock，保证异常安全
 *
 * Version:       2.0.0
 * Last Modified: 2026-06-15
 * =============================================================================
 */

#pragma once

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <mutex>
#include <string>
#include <tuple>
#include <type_traits>

#define LOGD(...) Logger::instance().log(LogLevel::DEBUG, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOGI(...) Logger::instance().log(LogLevel::INFO, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOGW(...) Logger::instance().log(LogLevel::WARNING, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOGE(...) Logger::instance().log(LogLevel::ERROR, __FILE__, __LINE__, ##__VA_ARGS__)
#define LOGF(...) Logger::instance().log(LogLevel::FATAL, __FILE__, __LINE__, ##__VA_ARGS__)

enum class LogLevel : std::size_t
{
    DEBUG   = 0,  // 调试信息，仅开发阶段使用
    INFO    = 1,  // 一般运行时信息
    WARNING = 2,  // 警告，不影响系统运行但值得关注
    ERROR   = 3,  // 错误，需要人工介入
    FATAL   = 4   // 严重错误，可能导致系统崩溃
};

// 日志级别字符串转换
const char* LogLevel_to_cstr(LogLevel level) noexcept;
LogLevel string_to_LogLevel(const std::string& level_str) noexcept;

struct LogConfig
{
    std::string file_name;                   // 日志文件路径，为空则仅输出到控制台
    LogLevel    level     = LogLevel::INFO;  // 默认info
    bool        console   = true;            // 是否输出到控制台
    size_t      max_lines = 500;             // 单文件最大行数，超过自动轮转
    size_t      buf_size  = 4096;            // 内部缓冲区大小（字节）

    static LogConfig console_only(LogLevel lv = LogLevel::INFO)
    {
        LogConfig c;
        c.level     = lv;
        c.console   = true;
        c.max_lines = 0;  // 纯控制台模式不轮转
        return c;
    }

    static LogConfig file_only(const std::string& path, LogLevel lv = LogLevel::INFO,
                               size_t max_lines = 500)
    {
        LogConfig c;
        c.file_name = path;
        c.level     = lv;
        c.console   = false;
        c.max_lines = max_lines;
        return c;
    }
};

class Logger
{
public:
    // 禁止拷贝与移动
    Logger(const Logger&)            = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&)                 = delete;
    Logger& operator=(Logger&&)      = delete;

    static Logger& instance() noexcept;

    void init(const LogConfig& config);
    void shutdown();

    void set_level(LogLevel level);
    void set_console(bool enable);
    void set_max_lines(size_t max_lines);

    bool set_level_str(const std::string& level_str);

    LogLevel level() const noexcept;
    bool console() const noexcept;
    size_t max_lines() const noexcept;
    size_t current_lines() const noexcept;

    template <typename... Args>
    void log(LogLevel level, const std::string& file, int line, Args&&... args);

private:
    Logger() = default;
    ~Logger();

    void flush_buffer();
    void rotate();

    // 时间戳格式化（线程安全）
    static int format_timestamp(char* buf, size_t buf_size);

    template <typename... Args>
    void append_args(char* buf, size_t& offset, size_t buf_size, Args&&... args);

    template <typename T>
    static void write_arg(char* buf, size_t& offset, size_t buf_size, T&& arg);

    void log_vprintf(char* buf, size_t& offset, size_t buf_size, const char* fmt, ...);

private:
    LogConfig   m_config;
    std::string m_file_name;

    std::ofstream m_fout;
    std::mutex    m_mutex;

    static constexpr size_t STACK_BUF_SIZE           = 4096;
    char                    m_buffer[STACK_BUF_SIZE] = {};
    size_t                  m_buf_offset             = 0;
    size_t                  m_cur_lines              = 0;
    bool                    m_initialized            = false;
};

template <typename... Args>
inline void Logger::log(LogLevel level, const std::string& file, int line, Args&&... args)
{
    if (static_cast<int>(level) < static_cast<int>(m_config.level))
        return;

    if (m_config.max_lines > 0 && m_cur_lines >= m_config.max_lines)
        rotate();

    char   stack_buf[STACK_BUF_SIZE];
    size_t offset = 0;

    offset += static_cast<size_t>(format_timestamp(stack_buf + offset, STACK_BUF_SIZE - offset));

    int written = snprintf(stack_buf + offset, STACK_BUF_SIZE - offset, " [%s] %s:%d ",
                           LogLevel_to_cstr(level), file.c_str(), line);
    if (written > 0)
        offset += static_cast<size_t>(written);

    if constexpr (sizeof...(Args) > 0)
    {
        using FirstArg     = std::tuple_element_t<0, std::tuple<Args...>>;
        using FirstDecayed = std::decay_t<FirstArg>;

        if constexpr (std::is_same_v<FirstDecayed, const char*> ||
                      std::is_same_v<FirstDecayed, char*>)
        {
            bool has_format =
                (strchr(std::get<0>(std::make_tuple(std::forward<Args>(args)...)), '%') != nullptr);

            if (has_format && sizeof...(Args) > 1)
            {
                std::apply(
                    [this, &stack_buf, &offset](auto&&... args_inner)
                    {
                        log_vprintf(stack_buf, offset, STACK_BUF_SIZE - offset,
                                    std::forward<decltype(args_inner)>(args_inner)...);
                    },
                    std::make_tuple(std::forward<Args>(args)...));
            }
            else
            {
                append_args(stack_buf, offset, STACK_BUF_SIZE - offset,
                            std::forward<Args>(args)...);
            }
        }
        else if constexpr (std::is_same_v<FirstDecayed, std::string>)
        {
            const std::string& first = std::get<0>(std::make_tuple(std::forward<Args>(args)...));
            bool               has_format = (first.find('%') != std::string::npos);

            if (has_format && sizeof...(Args) > 1)
            {
                std::apply(
                    [this, &stack_buf, &offset](auto&&... args_inner)
                    {
                        log_vprintf(stack_buf, offset, STACK_BUF_SIZE - offset,
                                    std::forward<decltype(args_inner)>(args_inner)...);
                    },
                    std::make_tuple(std::forward<Args>(args)...));
            }
            else
            {
                append_args(stack_buf, offset, STACK_BUF_SIZE - offset,
                            std::forward<Args>(args)...);
            }
        }
        else
        {
            append_args(stack_buf, offset, STACK_BUF_SIZE - offset, std::forward<Args>(args)...);
        }
    }

    if (offset < STACK_BUF_SIZE - 2)
    {
        stack_buf[offset++] = '\n';
        stack_buf[offset]   = '\0';
    }
    else
    {
        stack_buf[STACK_BUF_SIZE - 2] = '\n';
        stack_buf[STACK_BUF_SIZE - 1] = '\0';
        offset                        = STACK_BUF_SIZE - 1;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_config.console)
            std::fwrite(stack_buf, 1, offset, stdout);
        if (m_fout.is_open())
        {
            if (m_buf_offset + offset > STACK_BUF_SIZE)
                flush_buffer();

            std::memcpy(m_buffer + m_buf_offset, stack_buf, offset);
            m_buf_offset += offset;

            if (static_cast<int>(level) >= static_cast<int>(LogLevel::ERROR) ||
                m_buf_offset >= STACK_BUF_SIZE / 2)
            {
                flush_buffer();
            }
        }

        m_cur_lines++;
    }
}

template <typename T>
inline void Logger::write_arg(char* buf, size_t& offset, size_t buf_size, T&& arg)
{
    if (offset >= buf_size)
        return;

    using Decayed = std::decay_t<T>;

    if constexpr (std::is_same_v<Decayed, std::string>)
    {
        int w = snprintf(buf + offset, buf_size - offset, "%s", arg.c_str());
        if (w > 0)
            offset += static_cast<size_t>(w);
    }
    else if constexpr (std::is_same_v<Decayed, const char*> || std::is_same_v<Decayed, char*>)
    {
        int w = snprintf(buf + offset, buf_size - offset, "%s", arg);
        if (w > 0)
            offset += static_cast<size_t>(w);
    }
    else if constexpr (std::is_integral_v<Decayed>)
    {
        if constexpr (std::is_signed_v<Decayed>)
        {
            int w = snprintf(buf + offset, buf_size - offset, "%lld", static_cast<long long>(arg));
            if (w > 0)
                offset += static_cast<size_t>(w);
        }
        else
        {
            int w = snprintf(buf + offset, buf_size - offset, "%llu",
                             static_cast<unsigned long long>(arg));
            if (w > 0)
                offset += static_cast<size_t>(w);
        }
    }
    else if constexpr (std::is_floating_point_v<Decayed>)
    {
        int w = snprintf(buf + offset, buf_size - offset, "%g", static_cast<double>(arg));
        if (w > 0)
            offset += static_cast<size_t>(w);
    }
    else
    {
        // 非POD
        int w =
            snprintf(buf + offset, buf_size - offset, "%p", reinterpret_cast<const void*>(&arg));
        if (w > 0)
            offset += static_cast<size_t>(w);
    }
}

template <typename... Args>
inline void Logger::append_args(char* buf, size_t& offset, size_t buf_size, Args&&... args)
{
    // C++17 折叠表达式：依次处理每个参数
    (write_arg(buf, offset, buf_size, std::forward<Args>(args)), ...);
}