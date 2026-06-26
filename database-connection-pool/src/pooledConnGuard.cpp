#include "../include//conn-pool/pooledConnGuard.h"

/// 从连接池借出连接
/// @param pool 连接池引用
/// @param conn 借出的连接（调用方确保非空）
PooledConnGuard::PooledConnGuard(ConnectionPool* pool, IDatabaseConnection* conn)
    : m_pool(pool), m_conn(conn), m_released(false)
{
}

/// 移动构造
PooledConnGuard::PooledConnGuard(PooledConnGuard&& other) noexcept
    : m_pool(other.m_pool), m_conn(other.m_conn), m_released(other.m_released)
{
    other.m_pool     = nullptr;
    other.m_conn     = nullptr;
    other.m_released = false;
}

/// 移动赋值
PooledConnGuard& PooledConnGuard::operator=(PooledConnGuard&& other) noexcept
{
    if (this == &other)
        return *this;

    m_pool     = other.m_pool;
    m_conn     = other.m_conn;
    m_released = other.m_released;

    other.m_pool     = nullptr;
    other.m_conn     = nullptr;
    other.m_released = false;

    return *this;
}

/// 析构时自动归还连接
PooledConnGuard::~PooledConnGuard()
{
    release();
}

/// 获取持有的连接指针
IDatabaseConnection* PooledConnGuard::get() const
{
    return m_conn;
}

/// 解引用操作符
IDatabaseConnection& PooledConnGuard::operator*() const
{
    return *m_conn;
}

/// 成员访问操作符
IDatabaseConnection* PooledConnGuard::operator->() const
{
    return m_conn;
}

/// 检查是否持有有效连接
PooledConnGuard::operator bool() const
{
    return (m_pool != nullptr && m_conn != nullptr && m_released);
}

/// 主动归还连接（提前归还，析构时不再重复归还）
void PooledConnGuard::release()
{
    // if (!m_released)
    // m_pool->return_connection(m_conn);
}