/// 数据库连接池（多数据库类型单例）
///
/// 设计要点：
/// 1. 通过 DatabaseType 枚举区分不同数据库类型
/// 2. 每种数据库类型最多存在一个连接池实例（单例）
/// 3. instance() 方法接受 DatabaseType 参数，返回对应类型的唯一实例
/// 4. 线程安全：instance() 内部使用互斥锁保护实例创建
///
/// 使用示例：
///   auto& mysql_pool = ConnectionPool::instance(DatabaseType::MySQL);
///   mysql_pool.init(cfg, mysql_factory);
///   auto guard = mysql_pool.get_connection();

#pragma once
#include <condition_variable>
#include <map>
#include <mutex>
#include <queue>
#include <unordered_map>

#include "../database/DatabaseType.h"
#include "../database/IConnectionFactory.h"
#include "../database/IDatabaseConnection.h"
#include "../monitor/IHealthChecker.h"
#include "../monitor/PoolStatistics.h"


class PooledConnGuard;

class ConnectionPool
{
    friend class PooledConnGuard;

public:
    // ---- 禁用拷贝与移动 ----
    ConnectionPool(const ConnectionPool& other)            = delete;
    ConnectionPool(ConnectionPool&& other)                 = delete;
    ConnectionPool& operator=(const ConnectionPool& other) = delete;
    ConnectionPool& operator=(ConnectionPool&& other)      = delete;

    /// 获取指定数据库类型的连接池单例
    /// @param type 数据库类型
    /// @return 该类型对应的唯一连接池实例引用
    static ConnectionPool& instance(DatabaseType type);

    // ---- 初始化 ----

    void init(const ConnectionConfig& cfg, const IConnectionFactory* factory);
    void init(const ConnectionConfig& cfg, const IConnectionFactory* factory, int min, int max,
              int idle_time, int timeout);

    // ---- 连接借还 ----
    PooledConnGuard get_connection();

    // ---- 参数配置 ----
    void set_min_size(int val);
    void set_max_size(int val);
    void set_max_idle_time(int val);
    void set_timeout(int val);

    int get_min_size() const;
    int get_max_size() const;
    int get_max_idle_time() const;
    int get_timeout() const;

    // ---- 健康检查 ----
    /// 设置健康检查器（不同数据库类型可使用不同实现）
    void set_health_checker(IHealthChecker* checker);

    // ---- 统计信息 ----
    /// 获取连接池运行统计信息
    PoolStatistics get_statistics() const;

    // ---- 生命周期 ----
    /// 关闭连接池，释放所有连接
    void shutdown();

    /// 返回当前连接池对应的数据库类型
    DatabaseType get_database_type() const;

private:
    ConnectionPool(DatabaseType type);
    ~ConnectionPool();

    void return_connection(IDatabaseConnection* conn);

private:
    // ---- 多类型单例管理 ----
    static std::mutex                                        s_instance_mutex;
    static std::unordered_map<DatabaseType, ConnectionPool*> s_instances;

    // ---- 连接管理 ----
    std::queue<IDatabaseConnection*> m_conn_queue;
    std::mutex                       m_mutex;
    std::condition_variable          m_cv_not_empty;
    std::condition_variable          m_cv_not_full;

    // ---- 配置 ----
    DatabaseType              m_db_type;
    ConnectionConfig          m_cfg;
    const IConnectionFactory* m_factory;
    IHealthChecker*           m_health_checker;

    int  m_min_size;       // 连接池维护的最小连接数
    int  m_max_size;       // 连接池维护的最大连接数
    int  m_max_idle_time;  // 连接池中连接的最大空闲时长（秒）
    int  m_timeout;        // 连接池获取连接的超时时长（毫秒）
    bool m_initialized;    // 是否已完成初始化
};