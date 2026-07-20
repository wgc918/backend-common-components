//-----------------------------------------------------------------------------
// 文件: app.cpp
// 描述: App 主类实现 — 组件装配、Drogon 集成、生命周期管理
//-----------------------------------------------------------------------------

#include "app.h"

#include <drogon/drogon.h>

#include <iostream>

#include "conn-pool/ConnectionPool.h"
#include "database/DatabaseType.h"
#include "database/MySQLConnection.h"
#include "error_codes.h"
#include "ini.h"
#include "log.h"
#include "redis-pool/RedisConnectionPool.h"

namespace demo1
{

App::App()  = default;
App::~App() = default;

// ============================================================
// 配置加载
// ============================================================

bool App::load_config(const std::string& config_path)
{
    try
    {
        IniFile& ini = IniFile::instance();
        ini.load(config_path);

        auto& server   = ini["server"];
        auto& redis    = ini["redis"];
        auto& rpool    = ini["redis_pool"];
        auto& mysql    = ini["mysql"];
        auto& business = ini["business"];
        auto& log_sec  = ini["log"];

        m_config.server_port      = static_cast<int>(server["port"]);
        m_config.thread_pool_size = static_cast<int>(server["thread_pool_size"]);

        m_config.redis_host            = static_cast<std::string>(redis["host"]);
        m_config.redis_port            = static_cast<int>(redis["port"]);
        m_config.redis_password        = static_cast<std::string>(redis["password"]);
        m_config.redis_db              = static_cast<int>(redis["database"]);
        m_config.redis_connect_timeout = static_cast<int>(redis["connect_timeout"]);
        m_config.redis_socket_timeout  = static_cast<int>(redis["socket_timeout"]);

        m_config.redis_min_size      = static_cast<int>(rpool["max_idle"]);
        m_config.redis_max_size      = static_cast<int>(rpool["max_active"]);
        m_config.redis_max_idle_time = static_cast<int>(rpool["idle_timeout"]);
        m_config.redis_timeout       = 5000;
        m_config.redis_op_num        = static_cast<int>(rpool["op_num"]);

        m_config.mysql_host     = static_cast<std::string>(mysql["host"]);
        m_config.mysql_port     = static_cast<int>(mysql["port"]);
        m_config.mysql_user     = static_cast<std::string>(mysql["user"]);
        m_config.mysql_password = static_cast<std::string>(mysql["password"]);
        m_config.mysql_database = static_cast<std::string>(mysql["database"]);

        m_config.mysql_min_size      = static_cast<int>(mysql["max_idle_conns"]);
        m_config.mysql_max_size      = static_cast<int>(mysql["max_open_conns"]);
        m_config.mysql_max_idle_time = static_cast<int>(mysql["conn_max_lifetime"]);
        m_config.mysql_timeout       = 10000;
        m_config.mysql_op_num        = static_cast<int>(mysql["op_num"]);

        m_config.stock_key_prefix = static_cast<std::string>(business["stock_key_prefix"]);
        m_config.default_timeout  = static_cast<int>(business["default_timeout"]);
        m_config.retry_times      = static_cast<int>(business["retry_times"]);

        m_config.log_file_name = static_cast<std::string>(log_sec["file_name"]);
        m_config.log_level     = static_cast<std::string>(log_sec["level"]);
        m_config.log_console   = static_cast<bool>(log_sec["console"]);
        m_config.log_max_lines = static_cast<int>(log_sec["max_lines"]);
    }
    catch (const std::exception& e)
    {
        std::cerr << "[FATAL] Failed to load config: " << e.what() << std::endl;
        return false;
    }

    return true;
}

// ============================================================
// 日志初始化
// ============================================================

bool App::init_logger()
{
    LogConfig log_cfg;
    log_cfg.file_name = m_config.log_file_name;
    log_cfg.console   = m_config.log_console;
    log_cfg.max_lines = static_cast<size_t>(m_config.log_max_lines);

    Logger::instance().init(log_cfg);
    Logger::instance().set_level_str(m_config.log_level);

    LOGI("Logger initialized. level=%s, console=%d", m_config.log_level.c_str(),
         m_config.log_console);
    return true;
}

// ============================================================
// 数据库连接池初始化
// ============================================================

bool App::init_database_pool()
{
    dcp::ConnectionConfig cfg;
    cfg.db_name   = m_config.mysql_database;
    cfg.ip        = m_config.mysql_host;
    cfg.port      = static_cast<uint16_t>(m_config.mysql_port);
    cfg.user_name = m_config.mysql_user;
    cfg.password  = m_config.mysql_password;

    auto& pool = dcp::ConnectionPool::instance(dcp::DatabaseType::MySQL);
    pool.init(cfg, m_config.mysql_min_size, m_config.mysql_max_size,
              m_config.mysql_max_idle_time, m_config.mysql_timeout, m_config.mysql_op_num);

    LOGI("MySQL connection pool initialized. host=%s:%d, db=%s", m_config.mysql_host.c_str(),
         m_config.mysql_port, m_config.mysql_database.c_str());
    return true;
}

// ============================================================
// Redis 连接池初始化
// ============================================================

bool App::init_redis_pool()
{
    rcp::RedisConnectionConfig cfg;
    cfg.host            = m_config.redis_host;
    cfg.port            = m_config.redis_port;
    cfg.password        = m_config.redis_password;
    cfg.db              = m_config.redis_db;
    cfg.connect_timeout = m_config.redis_connect_timeout;
    cfg.socket_timeout  = m_config.redis_socket_timeout;

    auto& pool = rcp::RedisConnectionPool::instance();
    pool.init(cfg, m_config.redis_min_size, m_config.redis_max_size,
              m_config.redis_max_idle_time, m_config.redis_timeout, m_config.redis_op_num);

    LOGI("Redis connection pool initialized. host=%s:%d, db=%d", m_config.redis_host.c_str(),
         m_config.redis_port, m_config.redis_db);
    return true;
}

// ============================================================
// Bootstrap 数据初始化
// ============================================================

bool App::run_bootstrap()
{
    LOGI("Starting bootstrap: initializing inventory data...");
    auto result = m_bootstrap->run(1000, 10000);
    if (!result)
    {
        LOGE("Bootstrap failed: %s", result.unwrap_err().to_string().c_str());
        return false;
    }
    LOGI("Bootstrap completed successfully.");
    return true;
}

// ============================================================
// 控制器注册
// ============================================================

void App::register_controllers()
{
    auto ctrl = std::make_shared<InventoryController>(m_inventory_service);
    drogon::app().registerController(ctrl);
    LOGI("Drogon controllers registered.");
}

// ============================================================
// 公共接口
// ============================================================

bool App::init(const std::string& config_path)
{
    // 1. 加载配置
    if (!load_config(config_path))
    {
        return false;
    }

    // 2. 初始化日志
    if (!init_logger())
    {
        return false;
    }

    LOGI("=== Demo1 Inventory System Initializing ===");

    // 3. 注册自定义错误码（外部查表函数）
    ef::setExternalLookup(LookupError);

    // 4. 初始化数据库连接池
    if (!init_database_pool())
    {
        LOGE("Failed to initialize database pool");
        return false;
    }

    // 5. 初始化 Redis 连接池
    if (!init_redis_pool())
    {
        LOGE("Failed to initialize Redis pool");
        return false;
    }

    // 6. 创建组件（依赖注入，shared_ptr 共享生命周期）
    auto& mysql_pool = dcp::ConnectionPool::instance(dcp::DatabaseType::MySQL);
    auto& redis_pool = rcp::RedisConnectionPool::instance();

    m_repository = std::make_shared<InventoryRepository>(mysql_pool);
    m_inventory_service = std::make_shared<InventoryService>(
        redis_pool, *m_repository, m_config.stock_key_prefix, m_config.retry_times);
    m_bootstrap = std::make_shared<Bootstrap>(*m_inventory_service);

    m_initialized = true;
    LOGI("=== Application initialized successfully ===");
    return true;
}

void App::run()
{
    if (!m_initialized)
    {
        LOGE("App not initialized. Call init() first.");
        return;
    }

    // 执行 Bootstrap 数据初始化
    if (!run_bootstrap())
    {
        LOGE("Bootstrap failed, aborting.");
        return;
    }

    // 注册 Drogon 控制器
    register_controllers();

    // 配置 Drogon
    auto& app = drogon::app();
    app.addListener("0.0.0.0", static_cast<uint16_t>(m_config.server_port));
    app.setThreadNum(m_config.thread_pool_size);

    LOGI("Starting Drogon HTTP server on port %d with %d threads...",
         m_config.server_port, m_config.thread_pool_size);

    // 注册生命周期回调：停止时清理资源
    app.setPreShutdownAdvice(
        []()
        {
            LOGI("Drogon pre-shutdown: cleaning up resources...");
            rcp::RedisConnectionPool::instance().shutdown();
            dcp::ConnectionPool::instance(dcp::DatabaseType::MySQL).shutdown();
            Logger::instance().shutdown();
            LOGI("=== Application stopped ===");
        });

    // 阻塞运行
    app.run();
}

}  // namespace demo1