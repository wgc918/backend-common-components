//-----------------------------------------------------------------------------
// 文件: inventory_service.h
// 描述: 库存业务逻辑层 — 实现 "Redis 优先扣减，MySQL 事务持久化" 策略
//       提供接口契约与具体实现，支持依赖注入以解耦
//-----------------------------------------------------------------------------

#pragma once

#include <functional>
#include <memory>
#include <string>

#include "inventory_repository.h"
#include "redis_lua.h"
#include "redis-pool/RedisConnectionPool.h"
#include "redis-pool/RedisConnGuard.h"
#include "result.h"

namespace demo1
{

/// @brief 扣减请求结构
struct DeductRequest
{
    int64_t item_id;
    int     quantity;
};

/// @brief 扣减响应结构
struct DeductResponse
{
    int remaining_stock;
};

/// @brief 库存业务逻辑抽象接口
/// 定义核心业务操作的契约，便于单元测试
class IInventoryService
{
public:
    virtual ~IInventoryService() = default;

    /// @brief 初始化库存数据（Bootstrap 阶段调用）
    /// @param item_count 商品数量
    /// @param init_stock 每件初始库存
    virtual Result<void, ef::Error> initialize(int item_count, int init_stock) = 0;

    /// @brief 执行库存扣减
    /// @param req 扣减请求
    /// @return 成功时返回扣减响应，失败时返回 Error
    virtual Result<DeductResponse, ef::Error> deduct(const DeductRequest& req) = 0;

    /// @brief 查询 Redis 库存
    virtual Result<int, ef::Error> get_redis_stock(int64_t item_id) = 0;

    /// @brief 查询 MySQL 库存
    virtual Result<int, ef::Error> get_mysql_stock(int64_t item_id) = 0;
};

// ============================================================
// 具体实现
// ============================================================

/// @brief 库存服务实现
/// 采用 "Redis 优先扣减，MySQL 事务持久化" 策略
class InventoryService : public IInventoryService
{
public:
    /// @param redis_pool Redis 连接池
    /// @param repo       库存数据访问层
    /// @param key_prefix Redis Key 前缀（如 "stock:"）
    /// @param retry_times Redis 补偿重试次数
    InventoryService(rcp::RedisConnectionPool& redis_pool, IInventoryRepository& repo,
                     std::string key_prefix, int retry_times = 3);

    ~InventoryService() override = default;

    Result<void, ef::Error>           initialize(int item_count, int init_stock) override;
    Result<DeductResponse, ef::Error> deduct(const DeductRequest& req) override;
    Result<int, ef::Error>            get_redis_stock(int64_t item_id) override;
    Result<int, ef::Error>            get_mysql_stock(int64_t item_id) override;

private:
    /// @brief 构建 Redis Key
    std::string make_key(int64_t item_id) const;

    /// @brief Redis 降级到 MySQL 扣减
    Result<DeductResponse, ef::Error> fallback_to_mysql(const DeductRequest& req);

    /// @brief 补偿回滚 Redis（MySQL 异常时）
    Result<void, ef::Error> rollback_redis(const std::string& key, int qty);

    /// @brief 异步重建 Redis 缓存
    void async_rebuild_redis(int64_t item_id, int stock);

private:
    rcp::RedisConnectionPool&  m_redis_pool;
    IInventoryRepository& m_repo;
    std::string           m_key_prefix;
    int                   m_retry_times;
};

}  // namespace demo1