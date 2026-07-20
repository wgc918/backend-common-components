//-----------------------------------------------------------------------------
// 文件: consistency_checker.h
// 描述: 数据一致性校验模块 — 实时对比 Redis 和 MySQL 数据
//-----------------------------------------------------------------------------

#pragma once

#include <string>

#include "inventory_service.h"
#include "result.h"

namespace demo1
{

/// @brief 一致性检查响应结构
struct ConsistencyReport
{
    int64_t     item_id;
    int         redis_stock;
    int         mysql_stock;
    bool        consistent;
    bool        redis_available;
    std::string suggestion;
};

/// @brief 数据一致性校验器
class ConsistencyChecker
{
public:
    explicit ConsistencyChecker(IInventoryService& service);

    /// @brief 检查指定商品的数据一致性
    /// @param item_id 商品 ID
    /// @return 一致性检查报告
    Result<ConsistencyReport, ef::Error> check(int64_t item_id);

private:
    IInventoryService& m_service;
};

}  // namespace demo1