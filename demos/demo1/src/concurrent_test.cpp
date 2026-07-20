//-----------------------------------------------------------------------------
// 文件: concurrent_test.cpp
// 描述: 并发测试服务 — 多线程并发扣减验证
//-----------------------------------------------------------------------------

#include "concurrent_test.h"

#include <sstream>
#include <thread>

#include "log.h"

namespace demo1
{

ConcurrentTestService::ConcurrentTestService(IInventoryService& service) : m_service(service)
{
}

Result<ConcurrentTestResponse, ef::Error> ConcurrentTestService::run(const ConcurrentTestRequest& req)
{
    LOGI("Starting concurrent test: item_id=%lld, qty=%d, concurrency=%d",
         static_cast<long long>(req.item_id), req.quantity, req.concurrency);

    // 获取初始库存
    auto initial_stock_result = m_service.get_redis_stock(req.item_id);
    if (!initial_stock_result)
    {
        // 尝试从 MySQL 获取
        initial_stock_result = m_service.get_mysql_stock(req.item_id);
        if (!initial_stock_result)
        {
            return EF_ERROR(kConcurrencyTestFailed, "Cannot get initial stock for item_id=%lld",
                            static_cast<long long>(req.item_id));
        }
    }
    int initial_stock = *initial_stock_result;

    // 并发扣减
    std::atomic<int> success_count{0};
    std::atomic<int> fail_count{0};
    std::vector<std::thread> threads;
    threads.reserve(static_cast<size_t>(req.concurrency));

    for (int i = 0; i < req.concurrency; ++i)
    {
        threads.emplace_back(
            [this, &req, &success_count, &fail_count]()
            {
                DeductRequest dreq;
                dreq.item_id  = req.item_id;
                dreq.quantity = req.quantity;

                auto result = m_service.deduct(dreq);
                if (result)
                {
                    success_count.fetch_add(1, std::memory_order_relaxed);
                }
                else
                {
                    fail_count.fetch_add(1, std::memory_order_relaxed);
                }
            });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    // 获取最终库存
    auto final_redis = m_service.get_redis_stock(req.item_id);
    auto final_mysql = m_service.get_mysql_stock(req.item_id);

    int redis_stock = final_redis ? *final_redis : -1;
    int mysql_stock = final_mysql ? *final_mysql : -1;

    int theoretical_remain = initial_stock - (success_count.load() * req.quantity);
    bool data_consistent = (redis_stock == mysql_stock && redis_stock == theoretical_remain);

    ConcurrentTestResponse resp;
    resp.total_requests    = req.concurrency;
    resp.success_count     = success_count.load();
    resp.fail_count        = fail_count.load();
    resp.theoretical_remain = theoretical_remain;
    resp.actual_redis_stock = redis_stock;
    resp.actual_mysql_stock = mysql_stock;
    resp.data_consistent    = data_consistent;

    std::ostringstream summary;
    if (data_consistent)
    {
        summary << "PASS: Redis=" << redis_stock << ", MySQL=" << mysql_stock
                << ", Theoretical=" << theoretical_remain;
    }
    else
    {
        summary << "FAIL: Redis=" << redis_stock << ", MySQL=" << mysql_stock
                << ", Theoretical=" << theoretical_remain
                << ", Success=" << resp.success_count << ", Fail=" << resp.fail_count;
    }
    resp.summary = summary.str();

    LOGI("Concurrent test completed: %s", summary.str().c_str());
    return resp;
}

}  // namespace demo1