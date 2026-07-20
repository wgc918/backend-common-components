#include "../include/monitor/MySQLHealthChecker.h"

#include <jdbc/cppconn/connection.h>

#include <chrono>

#include "../include/database/MySQLConnection.h"

namespace dcp
{

MySQLHealthChecker::MySQLHealthChecker() : m_timeout_ms(5000)
{
}

HealthStatus MySQLHealthChecker::check(IDatabaseConnection* conn)
{
    if (conn == nullptr)
    {
        return HealthStatus::Unknown;
    }

    MySQLConnection* mysql_conn = dynamic_cast<MySQLConnection*>(conn);
    if (mysql_conn == nullptr)
    {
        return HealthStatus::Unknown;
    }

    auto start = std::chrono::steady_clock::now();

    auto res = mysql_conn->execute_query("select 1");

    auto end = std::chrono::steady_clock::now();

    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    if (res->next() && res->get_int(1) == 1 && duration_ms < m_timeout_ms)
        return HealthStatus::Healthy;
    else
        return HealthStatus::Unhealthy;
}

void MySQLHealthChecker::set_timeout_ms(int timeout_ms)
{
    m_timeout_ms = timeout_ms;
}

} // namespace dcp