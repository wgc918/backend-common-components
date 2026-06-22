#pragma once

#include "IHealthChecker.h"

/// MySQL 专用健康检查器
/// 通过执行 mysql_ping 或 SELECT 1 检测连接有效性
class MySQLHealthChecker : public IHealthChecker
{
public:
    MySQLHealthChecker();

    HealthStatus check(IDatabaseConnection* conn) override;

    void set_timeout_ms(int timeout_ms) override;

private:
    int m_timeout_ms;  // 健康检查超时时间（毫秒）
};