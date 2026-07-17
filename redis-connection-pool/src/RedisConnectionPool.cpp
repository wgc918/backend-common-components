#include "../include/redis-pool/RedisConnectionPool.h"

#include <iostream>
#include <thread>

#include "../include/redis-pool/RedisConnGuard.h"

// 静态成员变量定义
std::mutex           RedisConnectionPool::s_instance_mutex;
RedisConnectionPool* RedisConnectionPool::s_instance = nullptr;

/// 获取连接池单例
/// @return 全局唯一连接池实例引用
RedisConnectionPool& RedisConnectionPool::instance()
{
    std::lock_guard<std::mutex> lock(s_instance_mutex);

    if (s_instance == nullptr)
    {
        s_instance = new RedisConnectionPool();
    }

    return *s_instance;
}

/// 销毁连接池单例，释放所有资源
void RedisConnectionPool::destroy_instance()
{
    std::lock_guard<std::mutex> lock(s_instance_mutex);

    if (s_instance != nullptr)
    {
        s_instance->shutdown();
        delete s_instance;
        s_instance = nullptr;
    }
}

// ---- 构造与析构 ----

RedisConnectionPool::RedisConnectionPool()
    : m_cfg{},
      m_min_size(std::thread::hardware_concurrency()),
      m_max_size(m_min_size * 2),
      m_max_idle_time(60 * 3),
      m_timeout(5000),
      m_op_num(4),
      m_current_size(0),
      m_initialized(false),
      m_recycle_running(false)
{
    // 初始化统计信息
    m_stats.active_connections = 0;
    m_stats.idle_connections   = 0;
    m_stats.total_connections  = 0;
    m_stats.total_borrowed         = 0;
    m_stats.total_released         = 0;
    m_stats.total_created          = 0;
    m_stats.total_destroyed        = 0;
    m_stats.total_timeout_errors   = 0;
    m_stats.total_reconnect_count  = 0;
    m_stats.pending_waiters        = 0;
    m_stats.avg_wait_time_ms       = 0;
    m_stats.max_wait_time_ms       = 0;
    m_stats.created_at             = std::chrono::system_clock::now();
    m_stats.updated_at             = std::chrono::system_clock::now();
}

RedisConnectionPool::~RedisConnectionPool()
{
    shutdown();
}

// ---- 初始化 ----

void RedisConnectionPool::init(const RedisConnectionConfig& cfg)
{
    m_cfg         = cfg;
    m_initialized = true;

    {
        std::lock_guard<std::mutex> lock(m_stats_mutex);
        m_stats.created_at = std::chrono::system_clock::now();
    }

    source_connections();
    start_recycle_thread();
}

void RedisConnectionPool::init(const RedisConnectionConfig& cfg, int min, int max, int idle_time,
                                int timeout, int op_num)
{
    m_cfg           = cfg;
    m_initialized   = true;
    m_min_size      = min;
    m_max_size      = max;
    m_max_idle_time = idle_time;
    m_timeout       = timeout;
    m_op_num        = op_num;

    {
        std::lock_guard<std::mutex> lock(m_stats_mutex);
        m_stats.created_at = std::chrono::system_clock::now();
    }

    source_connections();
    start_recycle_thread();
}

// ---- 连接获取 ----

RedisConnGuard RedisConnectionPool::get_connection()
{
    if (!m_initialized)
    {
        std::cerr << "[RedisConnectionPool] Please call init() before get_connection()" << std::endl;
        return RedisConnGuard(this, nullptr);
    }

    std::unique_lock<std::mutex> locker(m_mutex);

    // 当前没有可用连接 且 连接数还没到达限制，自动扩容
    if (m_conn_queue.empty() && m_current_size < m_max_size)
    {
        expand_connections();
    }

    // 等待可用连接，带超时机制
    if (m_conn_queue.empty())
    {
        // 记录等待开始时间
        auto wait_start = std::chrono::steady_clock::now();

        {
            std::lock_guard<std::mutex> stats_lock(m_stats_mutex);
            m_stats.pending_waiters++;
        }

        auto status = m_cv_empty.wait_for(locker, std::chrono::milliseconds(m_timeout));

        {
            std::lock_guard<std::mutex> stats_lock(m_stats_mutex);
            m_stats.pending_waiters--;
        }

        // 超时仍未获取到连接
        if (status == std::cv_status::timeout && m_conn_queue.empty())
        {
            {
                std::lock_guard<std::mutex> stats_lock(m_stats_mutex);
                m_stats.total_timeout_errors++;
            }
            std::cerr << "[RedisConnectionPool] get_connection() timeout after " << m_timeout
                      << "ms" << std::endl;
            return RedisConnGuard(this, nullptr);
        }

        // 更新等待时间统计
        auto wait_end = std::chrono::steady_clock::now();
        auto wait_ms  = std::chrono::duration_cast<std::chrono::milliseconds>(wait_end - wait_start)
                           .count();
        {
            std::lock_guard<std::mutex> stats_lock(m_stats_mutex);
            if (m_stats.total_borrowed > 0)
            {
                m_stats.avg_wait_time_ms =
                    (m_stats.avg_wait_time_ms * (m_stats.total_borrowed - 1) + wait_ms) /
                    m_stats.total_borrowed;
            }
            else
            {
                m_stats.avg_wait_time_ms = wait_ms;
            }
            if (wait_ms > m_stats.max_wait_time_ms)
            {
                m_stats.max_wait_time_ms = wait_ms;
            }
        }
    }

    // 获取连接
    if (!m_conn_queue.empty())
    {
        auto entry = m_conn_queue.front();
        m_conn_queue.pop();

        {
            std::lock_guard<std::mutex> stats_lock(m_stats_mutex);
            m_stats.total_borrowed++;
            m_stats.active_connections++;
            m_stats.idle_connections = static_cast<int>(m_conn_queue.size());
            m_stats.updated_at       = std::chrono::system_clock::now();
        }

        RedisConnGuard guard(this, entry.first);
        return guard;
    }

    // 理论上不应该到达这里，但为了安全返回空守卫
    return RedisConnGuard(this, nullptr);
}

// ---- 连接归还 ----

void RedisConnectionPool::return_connection(sw::redis::Redis* conn)
{
    if (conn == nullptr)
    {
        return;
    }

    // 检查连接是否有效
    HealthStatus status = m_health_checker.check(conn);

    if (status == HealthStatus::Healthy)
    {
        // 连接有效，归还到队列
        std::lock_guard<std::mutex> lock(m_mutex);
        m_conn_queue.push(std::make_pair(conn, std::chrono::steady_clock::now()));
        m_cv_empty.notify_one();

        {
            std::lock_guard<std::mutex> stats_lock(m_stats_mutex);
            m_stats.total_released++;
            m_stats.active_connections--;
            m_stats.idle_connections = static_cast<int>(m_conn_queue.size());
            m_stats.updated_at       = std::chrono::system_clock::now();
        }
    }
    else
    {
        // 连接无效，销毁并尝试创建新连接补充
        destroy_connection(conn);

        std::lock_guard<std::mutex> lock(m_mutex);
        {
            std::lock_guard<std::mutex> stats_lock(m_stats_mutex);
            m_stats.total_reconnect_count++;
            m_stats.active_connections--;
            m_stats.total_released++;
        }

        // 如果当前连接数低于最小连接数，补充连接
        if (m_current_size < m_min_size)
        {
            try
            {
                sw::redis::Redis* new_conn = create_connection();
                if (new_conn != nullptr)
                {
                    m_conn_queue.push(
                        std::make_pair(new_conn, std::chrono::steady_clock::now()));
                    m_cv_empty.notify_one();
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "[RedisConnectionPool] Failed to create replacement connection: "
                          << e.what() << std::endl;
            }
        }

        {
            std::lock_guard<std::mutex> stats_lock(m_stats_mutex);
            m_stats.idle_connections = static_cast<int>(m_conn_queue.size());
            m_stats.updated_at       = std::chrono::system_clock::now();
        }
    }
}

// ---- 内部方法 ----

void RedisConnectionPool::source_connections()
{
    for (int i = 0; i < m_min_size; i++)
    {
        try
        {
            sw::redis::Redis* conn = create_connection();
            if (conn != nullptr)
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_conn_queue.push(std::make_pair(conn, std::chrono::steady_clock::now()));
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "[RedisConnectionPool] Failed to create initial connection " << i << ": "
                      << e.what() << std::endl;
        }
    }

    {
        std::lock_guard<std::mutex> stats_lock(m_stats_mutex);
        m_stats.idle_connections = static_cast<int>(m_conn_queue.size());
        m_stats.total_connections = m_current_size;
        m_stats.updated_at       = std::chrono::system_clock::now();
    }
}

void RedisConnectionPool::expand_connections()
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
        try
        {
            sw::redis::Redis* conn = create_connection();
            if (conn != nullptr)
            {
                m_conn_queue.push(std::make_pair(conn, std::chrono::steady_clock::now()));
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "[RedisConnectionPool] Failed to expand connection: " << e.what()
                      << std::endl;
        }
    }

    {
        std::lock_guard<std::mutex> stats_lock(m_stats_mutex);
        m_stats.idle_connections = static_cast<int>(m_conn_queue.size());
        m_stats.total_connections = m_current_size;
        m_stats.updated_at       = std::chrono::system_clock::now();
    }
}

sw::redis::Redis* RedisConnectionPool::create_connection()
{
    try
    {
        sw::redis::Redis* conn = m_factory.create_connection(m_cfg);
        m_current_size++;

        {
            std::lock_guard<std::mutex> stats_lock(m_stats_mutex);
            m_stats.total_created++;
        }

        return conn;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[RedisConnectionPool] Failed to create connection: " << e.what() << std::endl;
        throw;
    }
}

void RedisConnectionPool::destroy_connection(sw::redis::Redis* conn)
{
    if (conn != nullptr)
    {
        delete conn;
        m_current_size--;

        {
            std::lock_guard<std::mutex> stats_lock(m_stats_mutex);
            m_stats.total_destroyed++;
        }
    }
}

// ---- 回收线程 ----

void RedisConnectionPool::start_recycle_thread()
{
    if (m_recycle_running)
    {
        return;  // 已经在运行
    }

    m_recycle_running = true;
    m_recycle_thread  = std::thread([this]() {
        while (m_recycle_running)
        {
            // 每 30 秒检查一次
            std::this_thread::sleep_for(std::chrono::seconds(30));
            if (m_recycle_running)
            {
                recycle_idle_connections();
            }
        }
    });
}

void RedisConnectionPool::stop_recycle_thread()
{
    m_recycle_running = false;
    if (m_recycle_thread.joinable())
    {
        m_recycle_thread.join();
    }
}

void RedisConnectionPool::recycle_idle_connections()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto now = std::chrono::steady_clock::now();
    int  queue_size = static_cast<int>(m_conn_queue.size());

    // 计算可以回收的连接数（保留至少 m_min_size 个连接）
    int can_recycle = queue_size - m_min_size;
    if (can_recycle <= 0)
    {
        return;
    }

    int recycled = 0;
    for (int i = 0; i < queue_size && recycled < can_recycle; i++)
    {
        auto& entry = m_conn_queue.front();
        auto  idle_duration =
            std::chrono::duration_cast<std::chrono::seconds>(now - entry.second).count();

        if (idle_duration >= m_max_idle_time)
        {
            // 回收空闲过久的连接
            sw::redis::Redis* conn = entry.first;
            m_conn_queue.pop();
            destroy_connection(conn);
            recycled++;
        }
        else
        {
            // 把还没超时的连接放回队列末尾
            m_conn_queue.pop();
            m_conn_queue.push(entry);
        }
    }

    if (recycled > 0)
    {
        {
            std::lock_guard<std::mutex> stats_lock(m_stats_mutex);
            m_stats.idle_connections = static_cast<int>(m_conn_queue.size());
            m_stats.total_connections = m_current_size;
            m_stats.updated_at       = std::chrono::system_clock::now();
        }
        std::cout << "[RedisConnectionPool] Recycled " << recycled << " idle connections"
                  << std::endl;
    }
}

// ---- 参数配置 ----

void RedisConnectionPool::set_min_size(int val)
{
    m_min_size = val;
}

void RedisConnectionPool::set_max_size(int val)
{
    m_max_size = val;
}

void RedisConnectionPool::set_max_idle_time(int val)
{
    m_max_idle_time = val;
}

void RedisConnectionPool::set_timeout(int val)
{
    m_timeout = val;
}

int RedisConnectionPool::get_min_size() const
{
    return m_min_size;
}

int RedisConnectionPool::get_max_size() const
{
    return m_max_size;
}

int RedisConnectionPool::get_max_idle_time() const
{
    return m_max_idle_time;
}

int RedisConnectionPool::get_timeout() const
{
    return m_timeout;
}

// ---- 统计信息 ----

PoolStatistics RedisConnectionPool::get_statistics() const
{
    std::lock_guard<std::mutex> lock(m_stats_mutex);
    PoolStatistics stats = m_stats;
    return stats;
}

// ---- 生命周期 ----

void RedisConnectionPool::shutdown()
{
    // 先停止回收线程
    stop_recycle_thread();

    std::lock_guard<std::mutex> lock(m_mutex);

    // 释放队列中的所有连接
    while (!m_conn_queue.empty())
    {
        auto entry = m_conn_queue.front();
        m_conn_queue.pop();
        sw::redis::Redis* conn = entry.first;
        if (conn != nullptr)
        {
            delete conn;
        }
    }

    m_current_size = 0;
    m_initialized  = false;

    {
        std::lock_guard<std::mutex> stats_lock(m_stats_mutex);
        m_stats.active_connections = 0;
        m_stats.idle_connections   = 0;
        m_stats.total_connections  = 0;
        m_stats.updated_at         = std::chrono::system_clock::now();
    }
}
