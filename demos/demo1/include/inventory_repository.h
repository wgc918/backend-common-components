//-----------------------------------------------------------------------------
// 文件: inventory_repository.h
// 描述: 数据访问层抽象接口及 MySQL 实现
//       遵循依赖倒置原则：业务层依赖接口，不依赖具体实现
//-----------------------------------------------------------------------------

#pragma once

#include <memory>
#include <string>

#include "conn-pool/ConnectionPool.h"
#include "conn-pool/pooledConnGuard.h"
#include "error_codes.h"
#include "result.h"

namespace demo1
{

// ============================================================
// 接口定义
// ============================================================

/// @brief 库存数据访问抽象接口
/// 定义数据库层操作契约，便于单元测试 mock
class IInventoryRepository
{
public:
    virtual ~IInventoryRepository() = default;

    /// @brief 初始化单条库存记录（幂等）
    /// @param item_id 商品 ID
    /// @param stock   初始库存
    /// @return 成功时返回 void，失败时返回 Error
    virtual Result<void, ef::Error> init_stock(int64_t item_id, int stock) = 0;

    /// @brief 扣减库存（带悲观锁）
    /// @param item_id 商品 ID
    /// @param qty     扣减数量
    /// @return 成功时返回剩余库存，失败时返回 Error
    virtual Result<int, ef::Error> deduct_stock(int64_t item_id, int qty) = 0;

    /// @brief 查询库存
    /// @param item_id 商品 ID
    /// @return 成功时返回库存数量，失败时返回 Error
    virtual Result<int, ef::Error> get_stock(int64_t item_id) = 0;

    /// @brief 使用悲观锁查询库存（用于降级扣减）
    /// @param item_id 商品 ID
    /// @return 成功时返回库存数量，失败时返回 Error
    virtual Result<int, ef::Error> get_stock_for_update(int64_t item_id) = 0;

    /// @brief 直接更新库存值（用于降级扣减）
    /// @param item_id   商品 ID
    /// @param new_stock 新库存值
    /// @return 成功时返回 void，失败时返回 Error
    virtual Result<void, ef::Error> update_stock(int64_t item_id, int new_stock) = 0;
};

// ============================================================
// MySQL 实现
// ============================================================

/// @brief 基于 MySQL 连接池的库存数据访问实现
class InventoryRepository : public IInventoryRepository
{
public:
    explicit InventoryRepository(dcp::ConnectionPool& pool);
    ~InventoryRepository() override = default;

    Result<void, ef::Error> init_stock(int64_t item_id, int stock) override;
    Result<int, ef::Error>  deduct_stock(int64_t item_id, int qty) override;
    Result<int, ef::Error>  get_stock(int64_t item_id) override;
    Result<int, ef::Error>  get_stock_for_update(int64_t item_id) override;
    Result<void, ef::Error> update_stock(int64_t item_id, int new_stock) override;

private:
    dcp::ConnectionPool& m_pool;
};

}  // namespace demo1