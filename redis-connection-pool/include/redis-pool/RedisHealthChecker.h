#pragma once

#include <sw/redis++/redis++.h>

/// 健康检查状态
enum class HealthStatus
{
    Healthy,    // 连接正常
    Unhealthy,  // 连接异常，需要剔除
    Unknown,    // 无法判断
};

/// Redis 健康检查器
/// 通过 PING 命令检测 Redis 连接是否健康
class RedisHealthChecker
{
public:
    /// 对指定连接执行健康检查
    /// @param conn 待检查的 Redis 连接
    /// @return 健康状态
    HealthStatus check(sw::redis::Redis* conn);

    /// 设置健康检查超时时间（毫秒）
    void set_timeout_ms(int timeout_ms);

private:
    int m_timeout_ms = 3000;  // 默认 3 秒超时
};
