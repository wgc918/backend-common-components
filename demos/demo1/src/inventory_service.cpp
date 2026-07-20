//-----------------------------------------------------------------------------
// 文件: inventory_service.cpp
// 描述: 库存业务逻辑实现
//       "Redis 优先扣减，MySQL 事务持久化" 策略
//       异常时 Redis 回滚补偿
//-----------------------------------------------------------------------------

#include "inventory_service.h"

#include <thread>

#include "error_codes.h"
#include "log.h"

namespace demo1
{

// ============================================================
// 构造函数
// ============================================================

InventoryService::InventoryService(rcp::RedisConnectionPool& redis_pool, IInventoryRepository& repo,
                                   std::string key_prefix, int retry_times)
    : m_redis_pool(redis_pool), m_repo(repo), m_key_prefix(std::move(key_prefix)),
      m_retry_times(retry_times)
{
}

// ============================================================
// 辅助方法
// ============================================================

std::string InventoryService::make_key(int64_t item_id) const
{
    return m_key_prefix + std::to_string(item_id);
}

// ============================================================
// 初始化
// ============================================================

Result<void, ef::Error> InventoryService::initialize(int item_count, int init_stock)
{
    LOGI("Initializing %d items with stock=%d...", item_count, init_stock);

    auto redis_guard = m_redis_pool.get_connection();
    if (!redis_guard)
    {
        return EF_ERROR(kRedisUnavailable, "Redis unavailable during initialization");
    }

    for (int i = 1; i <= item_count; ++i)
    {
        int64_t item_id = static_cast<int64_t>(i);

        // 1. MySQL 幂等插入
        auto db_result = m_repo.init_stock(item_id, init_stock);
        if (!db_result)
        {
            LOGE("MySQL init failed for item %lld: %s", static_cast<long long>(item_id),
                 db_result.unwrap_err().to_string().c_str());
            return db_result;
        }

        // 2. 同步 Redis
        try
        {
            redis_guard->set(make_key(item_id), std::to_string(init_stock));
        }
        catch (const sw::redis::Error& e)
        {
            LOGE("Redis set failed for item %lld: %s", static_cast<long long>(item_id), e.what());
            return EF_ERROR(kRedisUnavailable, "Redis set failed for item %lld", static_cast<long long>(item_id));
        }

        if (i % 100 == 0)
        {
            LOGI("Initialized %d/%d items...", i, item_count);
        }
    }

    LOGI("Initialization completed. Total items: %d", item_count);
    return Result<void, ef::Error>();
}

// ============================================================
// 库存扣减
// ============================================================

Result<DeductResponse, ef::Error> InventoryService::deduct(const DeductRequest& req)
{
    if (req.item_id <= 0 || req.quantity <= 0)
    {
        return EF_ERROR(ef::errors::kInvalidArg, "Invalid item_id=%lld or quantity=%d",
                        static_cast<long long>(req.item_id), req.quantity);
    }

    std::string key = make_key(req.item_id);

    // 1. 获取 Redis 连接
    auto redis_guard = m_redis_pool.get_connection();
    if (!redis_guard)
    {
        LOGW("Redis unavailable, degrading to MySQL for item_id=%lld", static_cast<long long>(req.item_id));
        return fallback_to_mysql(req);
    }

    // 2. 执行 Lua 脚本原子扣减
    DeductResult lua_result = RedisLuaManager::execute_deduct(*redis_guard, key, req.quantity);

    switch (lua_result.result)
    {
    case LuaResult::KeyNotFound:
        LOGW("Redis key not found for item_id=%lld, degrading to MySQL", static_cast<long long>(req.item_id));
        return fallback_to_mysql(req);

    case LuaResult::StockInsufficient:
        LOGI("Stock insufficient: item_id=%lld, current=%d, need=%d",
             static_cast<long long>(req.item_id), lua_result.remaining_stock, req.quantity);
        return EF_ERROR(kStockInsufficient, "Insufficient stock: current=%d, need=%d",
                        lua_result.remaining_stock, req.quantity);

    case LuaResult::DeductSuccess:
    {
        // 3. 同步更新 MySQL
        auto db_result = m_repo.deduct_stock(req.item_id, req.quantity);
        if (!db_result)
        {
            LOGE("MySQL update failed after Redis deduct. Rolling back Redis. item_id=%lld",
                 static_cast<long long>(req.item_id));
            // 补偿回滚 Redis
            auto rollback_result = rollback_redis(key, req.quantity);
            if (!rollback_result)
            {
                LOGE("CRITICAL: Redis rollback also failed! item_id=%lld, qty=%d",
                     static_cast<long long>(req.item_id), req.quantity);
            }
            return EF_ERROR(kMySQLUpdateFailed, "MySQL sync failed: %s",
                            db_result.unwrap_err().to_string().c_str());
        }

        DeductResponse resp;
        resp.remaining_stock = lua_result.remaining_stock;
        LOGI("Deduct success: item_id=%lld, qty=%d, remain=%d",
             static_cast<long long>(req.item_id), req.quantity, resp.remaining_stock);
        return resp;
    }
    }

    return EF_ERROR(kLuaScriptFailed, "Unknown Lua result for item_id=%lld",
                    static_cast<long long>(req.item_id));
}

// ============================================================
// Redis 降级到 MySQL
// ============================================================

Result<DeductResponse, ef::Error> InventoryService::fallback_to_mysql(const DeductRequest& req)
{
    LOGW("Executing MySQL fallback for item_id=%lld, qty=%d",
         static_cast<long long>(req.item_id), req.quantity);

    auto db_result = m_repo.deduct_stock(req.item_id, req.quantity);
    if (!db_result)
    {
        return EF_ERROR(kMySQLUpdateFailed, "MySQL fallback deduct failed: %s",
                        db_result.unwrap_err().to_string().c_str());
    }

    int new_stock = *db_result;

    // 异步重建 Redis
    async_rebuild_redis(req.item_id, new_stock);

    DeductResponse resp;
    resp.remaining_stock = new_stock;
    return resp;
}

// ============================================================
// Redis 补偿回滚
// ============================================================

Result<void, ef::Error> InventoryService::rollback_redis(const std::string& key, int qty)
{
    for (int attempt = 0; attempt < m_retry_times; ++attempt)
    {
        auto guard = m_redis_pool.get_connection();
        if (!guard)
        {
            LOGW("Redis rollback attempt %d: Redis unavailable", attempt + 1);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        try
        {
            guard->incrby(key, qty);
            LOGI("Redis rollback success: key=%s, qty=%d", key.c_str(), qty);
            return Result<void, ef::Error>();
        }
        catch (const sw::redis::Error& e)
        {
            LOGW("Redis rollback attempt %d failed: %s", attempt + 1, e.what());
        }
    }

    return EF_ERROR(kCompensationFailed, "Redis rollback failed after %d retries for key=%s",
                    m_retry_times, key.c_str());
}

// ============================================================
// 异步重建 Redis 缓存
// ============================================================

void InventoryService::async_rebuild_redis(int64_t item_id, int stock)
{
    std::thread([this, item_id, stock]()
                {
                    try
                    {
                        auto guard = m_redis_pool.get_connection();
                        if (guard)
                        {
                            guard->set(make_key(item_id), std::to_string(stock));
                            LOGI("Async Redis rebuild: item_id=%lld, stock=%d",
                                 static_cast<long long>(item_id), stock);
                        }
                    }
                    catch (const std::exception& e)
                    {
                        LOGW("Async Redis rebuild failed for item_id=%lld: %s",
                             static_cast<long long>(item_id), e.what());
                    }
                })
        .detach();
}

// ============================================================
// 查询接口
// ============================================================

Result<int, ef::Error> InventoryService::get_redis_stock(int64_t item_id)
{
    auto guard = m_redis_pool.get_connection();
    if (!guard)
    {
        return EF_ERROR(kRedisUnavailable, "Redis unavailable for get_redis_stock");
    }

    try
    {
        auto val = guard->get(make_key(item_id));
        if (!val)
        {
            return EF_ERROR(kRedisKeyNotFound, "Redis key not found for item_id=%lld",
                            static_cast<long long>(item_id));
        }
        return std::stoi(*val);
    }
    catch (const sw::redis::Error& e)
    {
        return EF_ERROR(kRedisUnavailable, "Redis error: %s", e.what());
    }
}

Result<int, ef::Error> InventoryService::get_mysql_stock(int64_t item_id)
{
    return m_repo.get_stock(item_id);
}

}  // namespace demo1