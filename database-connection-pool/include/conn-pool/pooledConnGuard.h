#pragma once
#include <memory>

#include "../database/IDatabaseConnection.h"
#include "ConnectionPool.h"

/// 连接守卫（RAII）
/// 从连接池获取连接后，由 PooledConnGuard 持有；
/// 析构时自动将连接归还给连接池，防止连接泄漏。
/// 仅支持移动语义，禁止拷贝。
class PooledConnGuard
{
public:
    /// 从连接池借出连接
    /// @param pool 连接池引用
    /// @param conn 借出的连接（调用方确保非空）
    PooledConnGuard(ConnectionPool* pool, IDatabaseConnection* conn);

    /// 移动构造
    PooledConnGuard(PooledConnGuard&& other) noexcept;

    /// 移动赋值
    PooledConnGuard& operator=(PooledConnGuard&& other) noexcept;

    /// 禁止拷贝
    PooledConnGuard(const PooledConnGuard&)            = delete;
    PooledConnGuard& operator=(const PooledConnGuard&) = delete;

    /// 析构时自动归还连接
    ~PooledConnGuard();

    /// 获取持有的连接指针
    IDatabaseConnection* get() const;

    /// 解引用操作符
    IDatabaseConnection& operator*() const;

    /// 成员访问操作符
    IDatabaseConnection* operator->() const;

    /// 检查是否持有有效连接
    explicit operator bool() const;

    /// 主动归还连接（提前归还，析构时不再重复归还）
    void release();

private:
    ConnectionPool*      m_pool;      // 所属连接池
    IDatabaseConnection* m_conn;      // 持有的连接
    bool                 m_released;  // 是否已归还
};