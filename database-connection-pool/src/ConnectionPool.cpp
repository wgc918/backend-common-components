#include "../include/conn-pool/ConnectionPool.h"

#include <mutex>
#include <thread>

#include "../include/conn-pool/pooledConnGuard.h"
#include "../include/database/MySQLFactory.h"
#include "../include/monitor/MySQLHealthChecker.h"

// 静态成员变量定义
std::mutex                                        ConnectionPool::s_instance_mutex;
std::unordered_map<DatabaseType, ConnectionPool*> ConnectionPool::s_instances;

/// 获取指定数据库类型的连接池单例
/// @param type 数据库类型
/// @return 该类型对应的唯一连接池实例引用
ConnectionPool& ConnectionPool::instance(DatabaseType type)
{
    std::lock_guard<std::mutex> lock(s_instance_mutex);

    auto it = s_instances.find(type);
    if (it != s_instances.end())
    {
        return *it->second;
    }

    // 创建新实例并加入管理
    ConnectionPool* pool = new ConnectionPool(type);
    s_instances[type]    = pool;
    return *pool;
}

void ConnectionPool::destroy_instance(DatabaseType type)
{
    std::lock_guard<std::mutex> lock(s_instance_mutex);

    auto it = s_instances.find(type);
    if (it != s_instances.end())
    {
        it->second->shutdown();
        delete it->second;
        s_instances.erase(it);
    }
}

// ---- 初始化 ----

void ConnectionPool::init(const ConnectionConfig& cfg)
{
    m_cfg         = cfg;
    m_initialized = true;
    source_connections();
}

void ConnectionPool::init(const ConnectionConfig& cfg, int min, int max, int idle_time, int timeout,
                          int op_num)
{
    m_cfg           = cfg;
    m_initialized   = true;
    m_min_size      = min;
    m_max_size      = max;
    m_max_idle_time = idle_time;
    m_timeout       = timeout;
    m_op_num        = op_num;
    source_connections();
}

PooledConnGuard ConnectionPool::get_connection()
{
    if (!m_initialized)
    {
        std::cout << "Please call init() before get_connection()" << std::endl;
        return PooledConnGuard(this, nullptr);
    }

    std::unique_lock<std::mutex> locker(m_mutex);

    //  当前没有可用连接 且 连接数还没到达限制
    if (m_conn_queue.empty() && m_current_size < m_max_size)
    {
        expand_connections();
    }

    // 等待可用连接，带超时机制
    if (m_conn_queue.empty())
    {
        auto status = m_cv_empty.wait_for(locker, std::chrono::milliseconds(m_timeout));

        // 超时仍未获取到连接
        if (status == std::cv_status::timeout && m_conn_queue.empty())
        {
            // 真实业务场景可以在这里加一条日志
            return PooledConnGuard(this, nullptr);
        }
    }

    // 获取连接
    if (!m_conn_queue.empty())
    {
        PooledConnGuard ret(this, m_conn_queue.front());
        m_conn_queue.pop();
        return ret;
    }

    // 理论上不应该到达这里，但为了安全返回空守卫
    return PooledConnGuard(this, nullptr);
}

void ConnectionPool::source_connections()
{
    for (int i = 0; i < m_min_size; i++)
    {
        m_conn_queue.push(m_factory->create_connection(m_cfg));
        m_current_size++;
    }
}

void ConnectionPool::expand_connections()
{
    // 计算可以创建的新连接数
    int available = m_max_size - m_current_size;
    int to_create = std::min(available, m_op_num);

    if (to_create <= 0)
    {
        return;  // 已达到最大连接数，无法扩容
    }

    for (int i = 0; i < to_create; i++)
    {
        m_conn_queue.push(m_factory->create_connection(m_cfg));
        m_current_size++;
    }
}

// ---- 参数配置 ----
void ConnectionPool::set_min_size(int val)
{
    m_min_size = val;
}

void ConnectionPool::set_max_size(int val)
{
    m_max_size = val;
}

void ConnectionPool::set_max_idle_time(int val)
{
    m_max_idle_time = val;
}

void ConnectionPool::set_timeout(int val)
{
    m_timeout = val;
}

int ConnectionPool::get_min_size() const
{
    return m_min_size;
}

int ConnectionPool::get_max_size() const
{
    return m_max_size;
}

int ConnectionPool::get_max_idle_time() const
{
    return m_max_idle_time;
}

int ConnectionPool::get_timeout() const
{
    return m_timeout;
}

// ---- 统计信息 ----
/// 获取连接池运行统计信息
PoolStatistics ConnectionPool::get_statistics() const
{
    return PoolStatistics{};
}

// ---- 生命周期 ----
/// 关闭连接池，释放所有连接
void ConnectionPool::shutdown()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // 释放队列中的所有连接
    while (!m_conn_queue.empty())
    {
        IDatabaseConnection* conn = m_conn_queue.front();
        m_conn_queue.pop();
        if (conn != nullptr)
        {
            delete conn;
        }
    }

    m_current_size = 0;
    m_initialized  = false;
}

/// 返回当前连接池对应的数据库类型
DatabaseType ConnectionPool::get_database_type() const
{
    return m_db_type;
}

ConnectionPool::ConnectionPool(DatabaseType type)
    : m_db_type(type),
      m_cfg{},
      m_min_size(std::thread::hardware_concurrency()),
      m_max_size(m_min_size * 2),
      m_max_idle_time(60 * 3),
      m_timeout(10),
      m_op_num(4),
      m_current_size(0),
      m_initialized(false)
{
    switch (type)
    {
        case DatabaseType::MySQL:
        {
            m_factory = new MySQLFactory();
            // m_health_checker = new MySQLHealthChecker();
            break;
        }
        case DatabaseType::Oracle:
        {
            break;
        }
        case DatabaseType::PostgreSQL:
        {
            break;
        }
        case DatabaseType::SQLite:
        {
            break;
        }
        case DatabaseType::SQLServer:
        {
            break;
        }
    }
}

ConnectionPool::~ConnectionPool()
{
    if (m_factory != nullptr)
        delete m_factory;
    if (m_health_checker != nullptr)
        delete m_health_checker;
}

void ConnectionPool::return_connection(IDatabaseConnection* conn)
{
    if (conn != nullptr && conn->is_valid())
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_conn_queue.push(conn);
        m_cv_empty.notify_one();
    }
    else if (conn != nullptr)
    {
        // 连接无效，减少计数并删除连接
        std::lock_guard<std::mutex> lock(m_mutex);
        m_current_size--;
        delete conn;
    }
}