//-----------------------------------------------------------------------------
// 文件: consistency_checker.cpp
// 描述: 数据一致性校验器 — 实时对比 Redis 和 MySQL 数据
//-----------------------------------------------------------------------------

#include "consistency_checker.h"

#include "log.h"

namespace demo1
{

ConsistencyChecker::ConsistencyChecker(IInventoryService& service) : m_service(service)
{
}

Result<ConsistencyReport, ef::Error> ConsistencyChecker::check(int64_t item_id)
{
    ConsistencyReport report{};
    report.item_id          = item_id;
    report.redis_available  = true;

    auto redis_result = m_service.get_redis_stock(item_id);
    if (redis_result)
    {
        report.redis_stock = *redis_result;
    }
    else
    {
        report.redis_available = false;
        report.redis_stock     = -1;
        LOGW("Redis unavailable for consistency check: item_id=%lld", static_cast<long long>(item_id));
    }

    auto mysql_result = m_service.get_mysql_stock(item_id);
    if (mysql_result)
    {
        report.mysql_stock = *mysql_result;
    }
    else
    {
        report.mysql_stock = -1;
        LOGW("MySQL unavailable for consistency check: item_id=%lld", static_cast<long long>(item_id));
    }

    report.consistent = report.redis_available && (report.redis_stock == report.mysql_stock);

    if (!report.consistent)
    {
        if (!report.redis_available)
        {
            report.suggestion = "Redis unavailable. Check Redis service and connection.";
        }
        else
        {
            report.suggestion = "Data mismatch! Check compensation logs or manual repair needed.";
        }
        LOGW("Consistency check FAILED: item_id=%lld, Redis=%d, MySQL=%d",
             static_cast<long long>(item_id), report.redis_stock, report.mysql_stock);
    }
    else
    {
        LOGI("Consistency check PASSED: item_id=%lld, stock=%d",
             static_cast<long long>(item_id), report.redis_stock);
    }

    return report;
}

}  // namespace demo1