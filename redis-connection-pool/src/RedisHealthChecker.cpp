#include "../include/redis-pool/RedisHealthChecker.h"

HealthStatus RedisHealthChecker::check(sw::redis::Redis* conn)
{
    if (conn == nullptr)
    {
        return HealthStatus::Unhealthy;
    }

    // 发送 PING 命令检测连接是否正常
    auto reply = conn->ping();
    if (reply == "PONG")
    {
        return HealthStatus::Healthy;
    }
    return HealthStatus::Unknown;
}

void RedisHealthChecker::set_timeout_ms(int timeout_ms)
{
    m_timeout_ms = timeout_ms;
}
