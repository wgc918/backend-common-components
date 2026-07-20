#pragma once

#include <sw/redis++/redis++.h>

namespace rcp
{

class RedisConnectionPool;

/// Redis 连接守卫（RAII）
/// 从连接池获取连接后，由 RedisConnGuard 持有；
/// 析构时自动将连接归还给连接池，防止连接泄漏。
/// 仅支持移动语义，禁止拷贝。
class RedisConnGuard
{
public:
    /// 从连接池借出连接
    /// @param pool 连接池引用
    /// @param conn 借出的连接（调用方确保非空或为 nullptr 表示借出失败）
    RedisConnGuard(RedisConnectionPool* pool, sw::redis::Redis* conn);

    /// 移动构造
    RedisConnGuard(RedisConnGuard&& other) noexcept;

    /// 移动赋值
    RedisConnGuard& operator=(RedisConnGuard&& other) noexcept;

    /// 禁止拷贝
    RedisConnGuard(const RedisConnGuard&)            = delete;
    RedisConnGuard& operator=(const RedisConnGuard&) = delete;

    /// 析构时自动归还连接
    ~RedisConnGuard();

    /// 获取持有的连接指针
    sw::redis::Redis* get() const;

    /// 解引用操作符
    sw::redis::Redis& operator*() const;

    /// 成员访问操作符（可直接调用 Redis 命令，如 guard->set("key", "val")）
    sw::redis::Redis* operator->() const;

    /// 检查是否持有有效连接
    explicit operator bool() const;

    /// 主动归还连接（提前归还，析构时不再重复归还）
    void release();

private:
    RedisConnectionPool* m_pool;      // 所属连接池
    sw::redis::Redis*    m_conn;      // 持有的连接
    bool                 m_released;  // 是否已归还
};

} // namespace rcp
