/// Redis 连接池
///
/// 设计要点：
/// 1. 全局单例模式，管理所有 Redis 连接
/// 2. 线程安全：使用互斥锁 + 条件变量保护连接队列
/// 3. 自动扩容：当队列空虚且未达上限时自动创建新连接
/// 4. 自动重连：归还连接时检测有效性，无效连接自动销毁并重建
/// 5. 空闲回收：后台线程定期回收超时空闲连接
/// 6. RAII 守卫：通过 RedisConnGuard 自动归还连接，防止泄漏

#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>

#include <sw/redis++/redis++.h>

#include "PoolStatistics.h"
#include "RedisConnectionFactory.h"
#include "RedisHealthChecker.h"

class RedisConnGuard;

class RedisConnectionPool
{
    friend class RedisConnGuard;

public:
    // ---- 禁用拷贝与移动 ----
    RedisConnectionPool(const RedisConnectionPool& other)            = delete;
    RedisConnectionPool(RedisConnectionPool&& other)                 = delete;
    RedisConnectionPool& operator=(const RedisConnectionPool& other) = delete;
    RedisConnectionPool& operator=(RedisConnectionPool&& other)      = delete;

    /// 获取连接池单例
    /// @return 全局唯一连接池实例引用
    static RedisConnectionPool& instance();

    /// 销毁连接池单例，释放所有资源
    static void destroy_instance();

    // ---- 初始化 ----

    /// 使用默认参数初始化连接池
    /// @param cfg 连接配置
    void init(const RedisConnectionConfig& cfg);

    /// 使用自定义参数初始化连接池
    /// @param cfg       连接配置
    /// @param min       最小连接数
    /// @param max       最大连接数
    /// @param idle_time 最大空闲时间（秒）
    /// @param timeout   获取连接超时时间（毫秒）
    /// @param op_num    每次扩容数量
    void init(const RedisConnectionConfig& cfg, int min, int max, int idle_time, int timeout,
              int op_num);

    // ---- 连接借还 ----

    /// 从连接池获取一个连接（返回 RAII 守卫）
    /// @return 连接守卫，获取失败时 operator bool() 返回 false
    RedisConnGuard get_connection();

    // ---- 参数配置 ----
    /// 注意：以下方法非线程安全，应在主线程中调用

    void set_min_size(int val);
    void set_max_size(int val);
    void set_max_idle_time(int val);
    void set_timeout(int val);

    int get_min_size() const;
    int get_max_size() const;
    int get_max_idle_time() const;
    int get_timeout() const;

    // ---- 统计信息 ----

    /// 获取连接池运行统计信息
    PoolStatistics get_statistics() const;

    // ---- 生命周期 ----

    /// 关闭连接池，释放所有连接，停止回收线程
    void shutdown();

private:
    RedisConnectionPool();
    ~RedisConnectionPool();

    /// 归还连接到连接池（由 RedisConnGuard 调用）
    void return_connection(sw::redis::Redis* conn);

    /// 创建初始连接（达到最小连接数）
    void source_connections();

    /// 按需扩容连接池
    void expand_connections();

    /// 启动空闲连接回收线程
    void start_recycle_thread();

    /// 停止空闲连接回收线程
    void stop_recycle_thread();

    /// 回收超时空闲连接（由回收线程调用）
    void recycle_idle_connections();

    /// 创建一个新连接并更新统计信息
    sw::redis::Redis* create_connection();

    /// 销毁一个连接并更新统计信息
    void destroy_connection(sw::redis::Redis* conn);

private:

    // ---- 连接队列 ----
    /// 连接队列元素：连接指针 + 最后使用时间
    using ConnEntry = std::pair<sw::redis::Redis*, std::chrono::steady_clock::time_point>;
    std::queue<ConnEntry> m_conn_queue;
    std::mutex            m_mutex;
    std::condition_variable m_cv_empty;

    // ---- 配置 ----
    RedisConnectionConfig  m_cfg;
    RedisConnectionFactory m_factory;
    RedisHealthChecker     m_health_checker;

    int  m_min_size;       // 连接池维护的最小连接数
    int  m_max_size;       // 连接池维护的最大连接数
    int  m_max_idle_time;  // 连接池中连接的最大空闲时长（秒）
    int  m_timeout;        // 连接池获取连接的超时时长（毫秒）
    int  m_op_num;         // 一次增加或减少的连接数
    int  m_current_size;   // 当前总连接数（包括已借出和在队列中的）
    bool m_initialized;    // 是否已完成初始化

    // ---- 回收线程 ----
    std::thread     m_recycle_thread;
    std::atomic<bool> m_recycle_running;

    // ---- 统计信息 ----
    mutable std::mutex m_stats_mutex;
    PoolStatistics     m_stats;
};
