//-----------------------------------------------------------------------------
// 文件: app.h
// 描述: 应用主类 — 组件装配与生命周期管理
//       负责初始化所有组件、注册 Drogon 控制器、启动/停止服务
//-----------------------------------------------------------------------------

#pragma once

#include <memory>
#include <string>

#include "bootstrap.h"
#include "concurrent_test.h"
#include "consistency_checker.h"
#include "inventory_controller.h"
#include "inventory_repository.h"
#include "inventory_service.h"

namespace demo1
{

/// @brief 应用配置结构（从 config.ini 解析后填充）
struct AppConfig
{
    // 服务器
    int server_port          = 8080;
    int thread_pool_size     = 4;

    // Redis
    std::string redis_host            = "127.0.0.1";
    int         redis_port            = 6379;
    std::string redis_password;
    int         redis_db              = 0;
    int         redis_connect_timeout = 1000;
    int         redis_socket_timeout  = 1000;

    // Redis 连接池
    int redis_min_size       = 5;
    int redis_max_size       = 50;
    int redis_max_idle_time  = 300;
    int redis_timeout        = 5000;
    int redis_op_num         = 4;

    // MySQL
    std::string mysql_host     = "127.0.0.1";
    int         mysql_port     = 3306;
    std::string mysql_user     = "root";
    std::string mysql_password = "password";
    std::string mysql_database = "inventory_db";

    // MySQL 连接池
    int mysql_min_size      = 5;
    int mysql_max_size      = 20;
    int mysql_max_idle_time = 180;
    int mysql_timeout       = 10000;
    int mysql_op_num        = 4;

    // 业务
    std::string stock_key_prefix = "stock:";
    int         default_timeout  = 5;
    int         retry_times      = 3;

    // 日志
    std::string log_file_name = "logs/demo1.log";
    std::string log_level     = "INFO";
    bool        log_console   = true;
    int         log_max_lines = 500;
};

/// @brief 应用主类
/// 负责组件装配（依赖注入）与生命周期管理
class App
{
public:
    App();
    ~App();

    App(const App&)            = delete;
    App& operator=(const App&) = delete;

    /// @brief 从 INI 文件加载配置并初始化所有组件
    /// @param config_path INI 配置文件路径
    /// @return 成功返回 true
    bool init(const std::string& config_path);

    /// @brief 启动应用（初始化数据 + 启动 Drogon HTTP 服务）
    /// 此方法会阻塞直到服务停止
    void run();

private:
    /// @brief 加载配置
    bool load_config(const std::string& config_path);

    /// @brief 初始化日志系统
    bool init_logger();

    /// @brief 初始化数据库连接池
    bool init_database_pool();

    /// @brief 初始化 Redis 连接池
    bool init_redis_pool();

    /// @brief 执行 Bootstrap 数据初始化
    bool run_bootstrap();

    /// @brief 注册 Drogon 控制器
    void register_controllers();

private:
    AppConfig m_config;

    // 组件实例（shared_ptr，与 Drogon 控制器共享生命周期）
    std::shared_ptr<InventoryRepository> m_repository;
    std::shared_ptr<InventoryService>    m_inventory_service;
    std::shared_ptr<Bootstrap>           m_bootstrap;

    bool m_initialized = false;
};

}  // namespace demo1