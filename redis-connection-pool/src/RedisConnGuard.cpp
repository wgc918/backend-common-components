#include "../include/redis-pool/RedisConnGuard.h"
#include "../include/redis-pool/RedisConnectionPool.h"

/// 从连接池借出连接
/// @param pool 连接池引用
/// @param conn 借出的连接（调用方确保非空或为 nullptr 表示借出失败）
RedisConnGuard::RedisConnGuard(RedisConnectionPool* pool, sw::redis::Redis* conn)
    : m_pool(pool), m_conn(conn), m_released(false)
{
}

/// 移动构造
RedisConnGuard::RedisConnGuard(RedisConnGuard&& other) noexcept
    : m_pool(other.m_pool), m_conn(other.m_conn), m_released(other.m_released)
{
    other.m_pool     = nullptr;
    other.m_conn     = nullptr;
    other.m_released = false;
}

/// 移动赋值
RedisConnGuard& RedisConnGuard::operator=(RedisConnGuard&& other) noexcept
{
    if (this == &other)
        return *this;

    // 先归还当前持有的连接
    release();

    m_pool     = other.m_pool;
    m_conn     = other.m_conn;
    m_released = other.m_released;

    other.m_pool     = nullptr;
    other.m_conn     = nullptr;
    other.m_released = false;

    return *this;
}

/// 析构时自动归还连接
RedisConnGuard::~RedisConnGuard()
{
    release();
}

/// 获取持有的连接指针
sw::redis::Redis* RedisConnGuard::get() const
{
    return m_conn;
}

/// 解引用操作符
sw::redis::Redis& RedisConnGuard::operator*() const
{
    return *m_conn;
}

/// 成员访问操作符
sw::redis::Redis* RedisConnGuard::operator->() const
{
    return m_conn;
}

/// 检查是否持有有效连接
RedisConnGuard::operator bool() const
{
    return (m_pool != nullptr && m_conn != nullptr && !m_released);
}

/// 主动归还连接（提前归还，析构时不再重复归还）
void RedisConnGuard::release()
{
    if (!m_released)
    {
        if (m_pool != nullptr && m_conn != nullptr)
        {
            m_pool->return_connection(m_conn);
        }
        m_released = true;
        m_conn     = nullptr;
        m_pool     = nullptr;
    }
}
