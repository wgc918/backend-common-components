//-----------------------------------------------------------------------------
// 文件: concurrent_test.h
// 描述: 并发安全验证模块 — 支持多线程并发扣减测试
//-----------------------------------------------------------------------------

#pragma once

#include <atomic>
#include <string>
#include <vector>

#include "inventory_service.h"
#include "result.h"

namespace demo1
{

/// @brief 并发测试请求结构
struct ConcurrentTestRequest
{
    int64_t item_id;
    int     quantity;
    int     concurrency;
};

/// @brief 并发测试响应结构
struct ConcurrentTestResponse
{
    int         total_requests;
    int         success_count;
    int         fail_count;
    int         theoretical_remain;
    int         actual_redis_stock;
    int         actual_mysql_stock;
    bool        data_consistent;
    std::string summary;
};

/// @brief 并发测试服务
class ConcurrentTestService
{
public:
    explicit ConcurrentTestService(IInventoryService& service);

    /// @brief 执行并发扣减测试
    /// @param req 测试请求参数
    /// @return 测试结果统计
    Result<ConcurrentTestResponse, ef::Error> run(const ConcurrentTestRequest& req);

private:
    IInventoryService& m_service;
};

}  // namespace demo1