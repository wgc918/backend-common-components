#pragma once

#include "../database/IDatabaseConnection.h"

namespace dcp
{

/// 健康检查状态
enum class HealthStatus
{
    Healthy,    // 连接正常
    Unhealthy,  // 连接异常，需要剔除
    Unknown,    // 无法判断
};

/// 健康检查抽象接口
/// 不同数据库类型可实现各自的健康检查策略（如 MySQL 使用 mysql_ping，PostgreSQL 使用 pg_ping）
class IHealthChecker
{
public:
    virtual ~IHealthChecker() = default;

    /// 对指定连接执行健康检查
    /// @param conn 待检查的连接
    /// @return 健康状态
    virtual HealthStatus check(IDatabaseConnection* conn) = 0;

    /// 设置健康检查超时时间（毫秒）
    virtual void set_timeout_ms(int timeout_ms) = 0;
};

} // namespace dcp