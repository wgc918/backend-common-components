#include "../include/redis-pool/RedisHealthChecker.h"

namespace rcp
{

HealthStatus RedisHealthChecker::check(sw::redis::Redis* conn)
{
    if (conn == nullptr)
    {
        return HealthStatus::Unhealthy;
    }

    // 发送 PING 命令检测连接是否正常
    try
    {
        auto reply = conn->ping();
        if (reply == "PONG")
        {
            return HealthStatus::Healthy;
        }
    }
    catch (const sw::redis::Error& e)
    {
        return HealthStatus::Unhealthy;
    }

    return HealthStatus::Unknown;
}

void RedisHealthChecker::set_timeout_ms(int timeout_ms)
{
    m_timeout_ms = timeout_ms;
}

}  // namespace rcp
