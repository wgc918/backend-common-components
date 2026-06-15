/*
 * =============================================================================
 * Module:        logger 实现
 * Version:       2.0.0
 * =============================================================================
 */

#include "../include/log.h"

#include <sys/stat.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <ctime>
#include <iostream>

static constexpr std::array<const char*, 5> kLevelNames = {"DEBUG", "INFO", "WARNING", "ERROR",
                                                           "FATAL"};

const char* LogLevel_to_cstr(LogLevel level) noexcept
{
    auto idx = static_cast<size_t>(level);
    if (idx < kLevelNames.size())
        return kLevelNames[idx];
    return "UNKNOWN";
}

LogLevel string_to_LogLevel(const std::string& level_str) noexcept
{
    for (size_t i = 0; i < kLevelNames.size(); i++)
    {
        if (level_str == kLevelNames[i])
            return static_cast<LogLevel>(i);
    }
    return LogLevel::INFO;  // 默认
}

Logger& Logger::instance() noexcept
{
    static Logger inst;
    return inst;
}

void Logger::init(const LogConfig& config)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // 如果已经初始化过，先关闭
    if (m_initialized)
    {
        flush_buffer();
        if (m_fout.is_open())
            m_fout.close();
    }

    m_config      = config;
    m_file_name   = config.file_name;
    m_cur_lines   = 0;
    m_buf_offset  = 0;
    m_initialized = true;

    if (!m_file_name.empty())
    {
        std::ifstream in(m_file_name);
        if (in.is_open())
        {
            m_cur_lines = static_cast<size_t>(std::count(std::istreambuf_iterator<char>(in),
                                                         std::istreambuf_iterator<char>(), '\n'));
            in.close();
        }

        m_fout.open(m_file_name, std::ios::app);
        if (!m_fout.is_open())
        {
            std::fprintf(stderr, "[Logger] ERROR: Failed to open log file: %s\n",
                         m_file_name.c_str());
        }
    }
}

void Logger::shutdown()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_initialized)
    {
        flush_buffer();
        if (m_fout.is_open())
            m_fout.close();

        m_initialized = false;
    }
}

Logger::~Logger()
{
    // 析构时自动刷新，但不由析构函数关闭（因为是静态单例）
    // 用户应显式调用 shutdown() 以确保安全关闭
    if (m_buf_offset > 0 && m_fout.is_open())
    {
        m_fout.write(m_buffer, static_cast<std::streamsize>(m_buf_offset));
        m_fout.flush();
        m_buf_offset = 0;
    }
}

void Logger::set_level(LogLevel level)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config.level = level;
}

bool Logger::set_level_str(const std::string& level_str)
{
    LogLevel lv = string_to_LogLevel(level_str);
    // 验证转换有效性：如果输入不是已知级别且不是 "INFO"，说明无效
    // (string_to_LogLevel 对未匹配默认返回 INFO，需要额外判断)
    bool valid = true;
    for (size_t i = 0; i < kLevelNames.size(); i++)
    {
        if (level_str == kLevelNames[i])
        {
            valid = true;
            break;
        }
        valid = false;
    }

    if (valid)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_config.level = lv;
    }
    return valid;
}

LogLevel Logger::level() const noexcept
{
    return m_config.level;
}

bool Logger::console() const noexcept
{
    return m_config.console;
}

size_t Logger::max_lines() const noexcept
{
    return m_config.max_lines;
}

size_t Logger::current_lines() const noexcept
{
    return m_cur_lines;
}

void Logger::set_console(bool enable)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config.console = enable;
}

void Logger::set_max_lines(size_t max_lines)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config.max_lines = max_lines;
}

int Logger::format_timestamp(char* buf, size_t buf_size)
{
    auto now    = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm local_time;
    localtime_r(&time_t, &local_time);  // 线程安全

    return snprintf(buf, buf_size, "%04d-%02d-%02d %02d:%02d:%02d.%03lld",
                    local_time.tm_year + 1900, local_time.tm_mon + 1, local_time.tm_mday,
                    local_time.tm_hour, local_time.tm_min, local_time.tm_sec,
                    static_cast<long long>(ms.count()));
}

void Logger::flush_buffer()
{
    if (m_buf_offset == 0 || !m_fout.is_open())
        return;

    m_fout.write(m_buffer, static_cast<std::streamsize>(m_buf_offset));
    m_fout.flush();
    m_buf_offset = 0;
}

void Logger::rotate()
{
    if (m_file_name.empty())
        return;

    // 刷新未写入数据
    flush_buffer();

    if (m_fout.is_open())
        m_fout.close();

    // 生成带时间戳的备份文件名
    auto    now    = std::chrono::system_clock::now();
    auto    time_t = std::chrono::system_clock::to_time_t(now);
    std::tm local_time;
    localtime_r(&time_t, &local_time);

    char time_buf[32];
    strftime(time_buf, sizeof(time_buf), "%Y%m%d_%H%M%S", &local_time);

    std::string backup = m_file_name + "_" + time_buf + ".log";

    if (std::rename(m_file_name.c_str(), backup.c_str()) != 0)
    {
        // 重命名失败时尝试删除旧文件
        std::remove(m_file_name.c_str());
        std::fprintf(stderr, "[Logger] WARNING: rotate rename failed, removed: %s\n",
                     m_file_name.c_str());
    }

    // 重新打开文件
    m_cur_lines = 0;
    m_fout.open(m_file_name, std::ios::app);
    if (!m_fout.is_open())
    {
        std::fprintf(stderr, "[Logger] ERROR: rotate open failed: %s\n", m_file_name.c_str());
    }
}

void Logger::log_vprintf(char* buf, size_t& offset, size_t buf_size, const char* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    int written = vsnprintf(buf + offset, buf_size, fmt, va);
    va_end(va);
    if (written > 0)
        offset += static_cast<size_t>(written);
}