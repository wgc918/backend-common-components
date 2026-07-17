#include "../include/redis-pool/RedisHealthChecker.h"

#include <iostream>

HealthStatus RedisHealthChecker::check(sw::redis::Redis* conn)
{
    if (conn == nullptr)
    {
        return HealthStatus::Unhealthy;
    }

    try
    {
        // 发送 PING 命令检测连接是否正常
        auto reply = conn->ping();
        if (reply == "PONG")
        {
            return HealthStatus::Healthy;
        }
        return HealthStatus::Unknown;
    }
    catch (const sw::redis::Error& e)
    {
        // 捕获 redis-plus-plus 异常
        std::cerr << "[RedisHealthChecker] PING failed: " << e.what() << std::endl;
        return HealthStatus::Unhealthy;
    }
    catch (const std::exception& e)
    {
        // 捕获其他异常
        std::cerr << "[RedisHealthChecker] Unexpected error: " << e.what() << std::endl;
        return HealthStatus::Unhealthy;
    }
}

void RedisHealthChecker::set_timeout_ms(int timeout_ms)
{
    m_timeout_ms = timeout_ms;
}
