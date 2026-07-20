//-----------------------------------------------------------------------------
// 文件: inventory_repository.cpp
// 描述: MySQL 库存数据访问层实现
//-----------------------------------------------------------------------------

#include "inventory_repository.h"

#include "log.h"

namespace demo1
{

// ============================================================
// InventoryRepository 实现
// ============================================================

InventoryRepository::InventoryRepository(dcp::ConnectionPool& pool) : m_pool(pool)
{
}

Result<void, ef::Error> InventoryRepository::init_stock(int64_t item_id, int stock)
{
    auto guard = m_pool.get_connection();
    if (!guard)
    {
        return EF_ERROR(kMySQLUnavailable, "Failed to get MySQL connection for init item_id=%lld",
                        static_cast<long long>(item_id));
    }

    try
    {
        // 幂等插入：存在则忽略
        guard->prepare_statement(
            "INSERT INTO inventory (item_id, stock) VALUES (?, ?) "
            "ON DUPLICATE KEY UPDATE stock=stock");
        guard->set_int64(1, item_id);
        guard->set_int(2, stock);
        guard->execute_update();
    }
    catch (const std::exception& e)
    {
        return EF_ERROR(kMySQLUpdateFailed, "init_stock failed for item_id=%lld: %s",
                        static_cast<long long>(item_id), e.what());
    }

    return Result<void, ef::Error>();
}

Result<int, ef::Error> InventoryRepository::deduct_stock(int64_t item_id, int qty)
{
    auto guard = m_pool.get_connection();
    if (!guard)
    {
        return EF_ERROR(kMySQLUnavailable, "Failed to get MySQL connection for deduct item_id=%lld",
                        static_cast<long long>(item_id));
    }

    try
    {
        guard->begin();

        // 使用悲观锁防止并发更新丢失
        guard->prepare_statement(
            "SELECT stock FROM inventory WHERE item_id = ? FOR UPDATE");
        guard->set_int64(1, item_id);
        auto* rs = guard->execute_query();

        if (!rs || !rs->next())
        {
            guard->rollback();
            return EF_ERROR(kMySQLUpdateFailed, "item_id=%lld not found in MySQL",
                            static_cast<long long>(item_id));
        }

        int current = rs->get_int("stock");
        if (current < qty)
        {
            guard->rollback();
            return EF_ERROR(kStockInsufficient, "MySQL stock insufficient: have %d, need %d",
                            current, qty);
        }

        int new_stock = current - qty;
        guard->prepare_statement(
            "UPDATE inventory SET stock = ? WHERE item_id = ?");
        guard->set_int(1, new_stock);
        guard->set_int64(2, item_id);
        guard->execute_update();

        guard->commit();
        return new_stock;
    }
    catch (const std::exception& e)
    {
        try
        {
            guard->rollback();
        }
        catch (...)
        {
        }
        return EF_ERROR(kMySQLUpdateFailed, "deduct_stock failed for item_id=%lld: %s",
                        static_cast<long long>(item_id), e.what());
    }
}

Result<int, ef::Error> InventoryRepository::get_stock(int64_t item_id)
{
    auto guard = m_pool.get_connection();
    if (!guard)
    {
        return EF_ERROR(kMySQLUnavailable, "Failed to get MySQL connection for get_stock");
    }

    try
    {
        guard->prepare_statement("SELECT stock FROM inventory WHERE item_id = ?");
        guard->set_int64(1, item_id);
        auto* rs = guard->execute_query();

        if (!rs || !rs->next())
        {
            return EF_ERROR(ef::errors::kNotFound, "item_id=%lld not found",
                            static_cast<long long>(item_id));
        }

        return rs->get_int("stock");
    }
    catch (const std::exception& e)
    {
        return EF_ERROR(kMySQLUpdateFailed, "get_stock failed: %s", e.what());
    }
}

Result<int, ef::Error> InventoryRepository::get_stock_for_update(int64_t item_id)
{
    auto guard = m_pool.get_connection();
    if (!guard)
    {
        return EF_ERROR(kMySQLUnavailable, "Failed to get MySQL connection for get_stock_for_update");
    }

    try
    {
        guard->begin();
        guard->prepare_statement(
            "SELECT stock FROM inventory WHERE item_id = ? FOR UPDATE");
        guard->set_int64(1, item_id);
        auto* rs = guard->execute_query();

        if (!rs || !rs->next())
        {
            guard->rollback();
            return EF_ERROR(ef::errors::kNotFound, "item_id=%lld not found",
                            static_cast<long long>(item_id));
        }
        return rs->get_int("stock");
        // 注意：调用方需要负责 commit/rollback
    }
    catch (const std::exception& e)
    {
        try
        {
            guard->rollback();
        }
        catch (...)
        {
        }
        return EF_ERROR(kMySQLUpdateFailed, "get_stock_for_update failed: %s", e.what());
    }
}

Result<void, ef::Error> InventoryRepository::update_stock(int64_t item_id, int new_stock)
{
    auto guard = m_pool.get_connection();
    if (!guard)
    {
        return EF_ERROR(kMySQLUnavailable, "Failed to get MySQL connection for update_stock");
    }

    try
    {
        guard->prepare_statement(
            "UPDATE inventory SET stock = ? WHERE item_id = ?");
        guard->set_int(1, new_stock);
        guard->set_int64(2, item_id);
        int rows = guard->execute_update();

        if (rows == 0)
        {
            return EF_ERROR(ef::errors::kNotFound, "item_id=%lld not found for update",
                            static_cast<long long>(item_id));
        }
        return Result<void, ef::Error>();
    }
    catch (const std::exception& e)
    {
        return EF_ERROR(kMySQLUpdateFailed, "update_stock failed: %s", e.what());
    }
}

}  // namespace demo1