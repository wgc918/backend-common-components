//-----------------------------------------------------------------------------
// 文件: bootstrap.h
// 描述: 应用启动引导模块 — 负责初始化库存基础数据
//       启动时自动检查并初始化 MySQL + Redis 数据
//-----------------------------------------------------------------------------

#pragma once

#include "error_codes.h"
#include "inventory_service.h"
#include "result.h"

namespace demo1
{

/// @brief 应用启动引导器
/// 负责在应用启动时初始化库存基础数据
class Bootstrap
{
public:
    explicit Bootstrap(IInventoryService& service);

    /// @brief 执行初始化
    /// @param item_count 商品总数（默认 1000）
    /// @param init_stock 每件初始库存（默认 10000）
    /// @return 成功时返回 void，失败时返回 Error
    Result<void, ef::Error> run(int item_count = 1000, int init_stock = 10000);

private:
    IInventoryService& m_service;
};

}  // namespace demo1