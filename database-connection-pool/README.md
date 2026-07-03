# Database Connection Pool

高性能、可扩展的C++数据库连接池库，支持多种数据库类型，提供线程安全的连接管理和智能生命周期控制。

## 目录

- [项目概述与目标](#项目概述与目标)
- [当前实现状态](#当前实现状态)
- [核心功能特性](#核心功能特性)
- [技术栈选型说明](#技术栈选型说明)
- [架构设计与模块说明](#架构设计与模块说明)
- [详细实现流程与关键代码解析](#详细实现流程与关键代码解析)
- [环境配置与依赖说明](#环境配置与依赖说明)
- [使用示例](#使用示例)
- [性能优化策略](#性能优化策略)
- [常见问题解决方案](#常见问题解决方案)
- [扩展开发指南](#扩展开发指南)

---

## 项目概述与目标

### 项目简介

本项目是一个基于C++17开发的通用数据库连接池库，采用现代化的设计模式和工程实践，旨在为后端服务提供高效、可靠的数据库连接管理能力。

### 设计目标

1. **高性能**: 最小化连接创建开销，支持高并发场景
2. **多数据库支持**: 抽象统一接口，便于扩展不同数据库类型
3. **线程安全**: 完善的并发控制机制，支持多线程环境
4. **智能管理**: 自动扩容、健康检查、超时控制等自动化特性
5. **易于使用**: 简洁的API设计，支持RAII自动资源管理
6. **可观测性**: 内置统计信息，便于监控和调优

---

## 当前实现状态

| 功能模块 | 状态 | 说明 |
|---------|------|------|
| MySQL连接实现 | ✅ 已实现 | 完整支持MySQL数据库连接、查询、事务操作 |
| 连接池核心逻辑 | ✅ 已实现 | 单例模式、自动扩容、超时获取、线程安全 |
| RAII连接管理 | ✅ 已实现 | `PooledConnGuard` 自动归还连接 |
| 预处理语句 | ✅ 已实现 | 支持参数化查询，防止SQL注入 |
| 事务控制 | ✅ 已实现 | begin/commit/rollback接口完整实现 |
| 健康检查接口 | ✅ 已实现 | `IHealthChecker` 和 `MySQLHealthChecker` 类已完成 |
| 健康检查集成 | ❌ 待实现 | 健康检查器尚未集成到连接池主流程 |
| 统计信息结构 | ✅ 已实现 | `PoolStatistics` 结构定义完整 |
| 统计信息采集 | ❌ 待实现 | `get_statistics()` 当前返回空结构 |
| 其他数据库支持 | ❌ 待实现 | SQLite/PostgreSQL/Oracle/SQLServer支持待开发 |
| 连接回收机制 | ❌ 待实现 | 基于最大空闲时间的连接回收尚未实现 |
| `last_insert_id()` | ❌ 待实现 | 当前固定返回0 |

> **注意**: `IResultSet` 对象需要调用方手动 `delete` 释放，存在潜在内存泄漏风险，后续版本将考虑引入智能指针。

---

## 核心功能特性

| 特性 | 描述 |
|------|------|
| **多数据库类型支持** | 抽象统一接口，便于扩展不同数据库类型（当前仅MySQL已实现） |
| **单例模式** | 每种数据库类型维护唯一连接池实例，避免资源浪费 |
| **连接池自动扩容** | 根据实际需求动态调整连接数量，平衡性能与资源消耗 |
| **连接超时机制** | 获取连接支持超时配置，避免线程无限等待 |
| **RAII连接管理** | 通过 `PooledConnGuard` 自动归还连接，防止连接泄漏 |
| **健康检查框架** | 提供健康检查抽象接口，便于扩展不同数据库的健康检查策略 |
| **事务支持** | 完整的事务控制接口（begin/commit/rollback） |
| **预处理语句** | 支持参数化查询，防止SQL注入 |
| **线程安全** | 使用互斥锁和条件变量保证并发安全 |

---

## 技术栈选型说明

### 语言选择

- **C++17**: 现代化语言特性，包括智能指针、移动语义、结构化绑定等，提升代码质量和性能

### 数据库驱动

- **MySQL Connector/C++**: Oracle官方提供的JDBC风格MySQL驱动，API稳定，功能完善

### 设计模式

- **单例模式**: 确保每种数据库类型只有一个连接池实例
- **工厂模式**: 通过 `IConnectionFactory` 创建不同数据库的连接对象
- **策略模式**: 健康检查和连接创建策略可灵活替换
- **RAII**: 通过 `PooledConnGuard` 实现自动资源管理
- **生产者-消费者**: 连接队列采用条件变量实现线程同步

---

## 架构设计与模块说明

### 架构设计图

```
┌─────────────────────────────────────────────────────────────────┐
│                    应用层 (Application Layer)                    │
│              PooledConnGuard → 业务逻辑 → IResultSet             │
└──────────────────────┬──────────────────────────────────────────┘
                       │ 获取连接 / 归还连接
                       ▼
┌─────────────────────────────────────────────────────────────────┐
│                   连接池层 (Connection Pool Layer)               │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                   ConnectionPool                          │  │
│  │  ┌──────────┐  ┌──────────┐  ┌─────────────────────────┐ │  │
│  │  │ 队列管理 │  │ 扩容策略 │  │    统计与监控模块        │ │  │
│  │  └────┬─────┘  └────┬─────┘  └─────────────────────────┘ │  │
│  │       │             │                                     │  │
│  │       ▼             ▼                                     │  │
│  │  ┌─────────────────────────────────────────────────────┐  │  │
│  │  │              std::queue<IDatabaseConnection*>       │  │  │
│  │  └─────────────────────────────────────────────────────┘  │  │
│  └───────────────────────────────────────────────────────────┘  │
└──────────────────────┬──────────────────────────────────────────┘
                       │ 创建连接
                       ▼
┌─────────────────────────────────────────────────────────────────┐
│                 连接工厂层 (Connection Factory Layer)            │
│            IConnectionFactory → MySQLFactory → ...               │
└──────────────────────┬──────────────────────────────────────────┘
                       │ 创建具体连接
                       ▼
┌─────────────────────────────────────────────────────────────────┐
│                数据库适配层 (Database Adapter Layer)              │
│  IDatabaseConnection → MySQLConnection                          │
│       IResultSet     → MySQLResultSet                           │
│    IHealthChecker    → MySQLHealthChecker                       │
└──────────────────────┬──────────────────────────────────────────┘
                       │ 底层驱动
                       ▼
┌─────────────────────────────────────────────────────────────────┐
│                    数据库层 (Database Layer)                     │
│                      MySQL / PostgreSQL / ...                    │
└─────────────────────────────────────────────────────────────────┘
```

### 模块说明

#### 1. 连接池核心模块 (`conn-pool`)

| 文件 | 职责 |
|------|------|
| `ConnectionPool.h/cpp` | 连接池核心类，管理连接队列、线程同步、扩容策略 |
| `pooledConnGuard.h/cpp` | RAII连接守卫，自动归还连接 |

#### 2. 数据库抽象模块 (`database`)

| 文件 | 职责 |
|------|------|
| `IDatabaseConnection.h` | 数据库连接抽象接口，定义连接、执行、事务等操作 |
| `IConnectionFactory.h` | 连接工厂抽象接口，定义连接创建方法 |
| `IResultSet.h` | 结果集抽象接口，定义数据读取方法 |
| `DatabaseType.h` | 数据库类型枚举 |
| `MySQLConnection.h/cpp` | MySQL连接实现 |
| `MySQLFactory.h/cpp` | MySQL连接工厂实现 |
| `MySQLResultSet.h/cpp` | MySQL结果集实现 |

#### 3. 监控模块 (`monitor`)

| 文件 | 职责 |
|------|------|
| `IHealthChecker.h` | 健康检查抽象接口 |
| `MySQLHealthChecker.h/cpp` | MySQL健康检查实现（尚未集成到连接池） |
| `PoolStatistics.h` | 连接池统计信息结构（尚未实现采集逻辑） |

---

## 详细实现流程与关键代码解析

### 1. 连接池初始化流程

```cpp
// ConnectionPool.cpp - 构造函数
ConnectionPool::ConnectionPool(DatabaseType type)
    : m_db_type(type),
      m_min_size(std::thread::hardware_concurrency()),  // 默认最小连接数 = CPU核心数
      m_max_size(m_min_size * 2),                        // 默认最大连接数 = 最小连接数 * 2
      m_max_idle_time(60 * 3),                           // 默认最大空闲时间 = 3分钟
      m_timeout(10),                                     // 默认超时时间 = 10毫秒
      m_op_num(4),                                       // 默认每次扩容数量 = 4
      m_current_size(0),
      m_initialized(false)
{
    // 根据数据库类型创建对应的工厂
    switch (type) {
        case DatabaseType::MySQL:
            m_factory = new MySQLFactory();
            break;
        // ... 其他数据库类型待实现
    }
}
```

**设计要点**:
- 构造函数初始化默认参数，基于硬件配置自动设置合理的连接池大小
- 使用工厂模式创建具体数据库连接，实现数据库类型的解耦

### 2. 获取连接流程

```cpp
// ConnectionPool.cpp - get_connection()
PooledConnGuard ConnectionPool::get_connection()
{
    if (!m_initialized) {
        return PooledConnGuard(this, nullptr);  // 未初始化，返回空守卫
    }

    std::unique_lock<std::mutex> locker(m_mutex);

    // 当前没有可用连接 且 连接数还没到达限制，自动扩容
    if (m_conn_queue.empty() && m_current_size < m_max_size) {
        expand_connections();
    }

    // 等待可用连接，带超时机制
    if (m_conn_queue.empty()) {
        auto status = m_cv_empty.wait_for(locker, std::chrono::milliseconds(m_timeout));
        if (status == std::cv_status::timeout && m_conn_queue.empty()) {
            return PooledConnGuard(this, nullptr);  // 超时返回空守卫
        }
    }

    // 获取连接
    if (!m_conn_queue.empty()) {
        PooledConnGuard ret(this, m_conn_queue.front());
        m_conn_queue.pop();
        return ret;
    }

    return PooledConnGuard(this, nullptr);
}
```

**设计要点**:
- 使用 `std::unique_lock` + `std::condition_variable` 实现线程安全的等待机制
- 支持超时获取，避免线程无限阻塞
- 自动扩容策略：当队列空且未达上限时，自动创建新连接

### 3. RAII连接管理

```cpp
// pooledConnGuard.cpp - 析构函数
PooledConnGuard::~PooledConnGuard()
{
    release();  // 析构时自动归还连接
}

// pooledConnGuard.cpp - release()
void PooledConnGuard::release()
{
    if (!m_released) {
        m_pool->return_connection(m_conn);
        m_released = true;
    }
}
```

**设计要点**:
- 使用RAII模式，连接守卫析构时自动归还连接，防止连接泄漏
- 禁止拷贝，仅支持移动语义，确保连接所有权唯一
- `release()` 方法支持主动归还，避免重复归还

### 4. 连接归还流程

```cpp
// ConnectionPool.cpp - return_connection()
void ConnectionPool::return_connection(IDatabaseConnection* conn)
{
    if (conn != nullptr && conn->is_valid()) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_conn_queue.push(conn);
        m_cv_empty.notify_one();  // 通知等待的线程
    } else if (conn != nullptr) {
        // 连接无效，减少计数并删除连接
        std::lock_guard<std::mutex> lock(m_mutex);
        m_current_size--;
        delete conn;
    }
}
```

**设计要点**:
- 归还时检查连接有效性，无效连接直接销毁
- 使用 `notify_one()` 唤醒等待的线程，避免不必要的广播

### 5. 自动扩容策略

```cpp
// ConnectionPool.cpp - expand_connections()
void ConnectionPool::expand_connections()
{
    int available = m_max_size - m_current_size;
    int to_create = std::min(available, m_op_num);  // 每次最多扩容 m_op_num 个连接

    if (to_create <= 0) {
        return;
    }

    for (int i = 0; i < to_create; i++) {
        m_conn_queue.push(m_factory->create_connection(m_cfg));
        m_current_size++;
    }
}
```

**设计要点**:
- 增量扩容，避免一次性创建过多连接导致资源浪费
- 扩容数量可配置 (`m_op_num`)，默认每次扩容4个连接

---

## 环境配置与依赖说明

### 系统要求

- **操作系统**: Linux / macOS / Windows
- **编译器**: GCC 7+ / Clang 6+ / MSVC 2017+
- **CMake**: 3.10+

### 依赖库

| 依赖 | 版本要求 | 获取方式 |
|------|----------|----------|
| MySQL Connector/C++ | 8.0+ | `sudo apt-get install libmysqlcppconn-dev` (Ubuntu) |

### 编译安装

```bash
# 创建构建目录
mkdir build && cd build

# 配置CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译
make -j$(nproc)

# 安装（可选）
sudo make install
```

---

## 使用示例

### 1. 基础使用（直接使用连接）

```cpp
#include "database/MySQLConnection.h"

int main() {
    // 配置连接参数
    ConnectionConfig cfg;
    cfg.db_name   = "testdb";
    cfg.ip        = "127.0.0.1";
    cfg.port      = 3306;
    cfg.user_name = "username";
    cfg.password  = "password";

    // 创建连接
    MySQLConnection conn(cfg);

    // 执行DDL
    conn.execute("CREATE TABLE users (id INT PRIMARY KEY, name VARCHAR(100));");

    // 执行INSERT（返回受影响行数）
    int affected = conn.execute_update("INSERT INTO users VALUES (1, 'Alice');");

    // 执行SELECT（返回结果集）
    IResultSet* res = conn.execute_query("SELECT * FROM users;");
    while (res->next()) {
        std::cout << res->get_int("id") << " " << res->get_string("name") << std::endl;
    }
    delete res;  // 注意：结果集需要手动释放

    // 关闭连接
    conn.close();

    return 0;
}
```

### 2. 使用连接池（推荐方式）

```cpp
#include "conn-pool/ConnectionPool.h"
#include "conn-pool/pooledConnGuard.h"

void do_work(int thread_id) {
    // 从连接池获取连接（RAII自动管理）
    auto guard = ConnectionPool::instance(DatabaseType::MySQL).get_connection();

    if (!guard) {
        std::cerr << "Failed to get connection" << std::endl;
        return;
    }

    // 使用连接（通过guard访问）
    guard->execute("CREATE TABLE IF NOT EXISTS test_table (id INT);");
    guard->execute_update("INSERT INTO test_table VALUES (1);");

    IResultSet* res = guard->execute_query("SELECT COUNT(*) FROM test_table;");
    if (res->next()) {
        std::cout << "Count: " << res->get_int(1) << std::endl;
    }
    delete res;

    // guard析构时自动归还连接，无需手动close
}

int main() {
    // 配置连接参数
    ConnectionConfig cfg;
    cfg.db_name   = "testdb";
    cfg.ip        = "127.0.0.1";
    cfg.port      = 3306;
    cfg.user_name = "username";
    cfg.password  = "password";

    // 初始化连接池（最小5个连接，最大20个连接，空闲超时180秒，获取超时10秒）
    auto& pool = ConnectionPool::instance(DatabaseType::MySQL);
    pool.init(cfg, 5, 20, 180, 10000, 4);

    // 多线程并发测试
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.emplace_back(do_work, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    // 关闭连接池
    pool.shutdown();

    return 0;
}
```

### 3. 使用预处理语句（防止SQL注入）

```cpp
#include "conn-pool/ConnectionPool.h"

void insert_user(int id, const std::string& name) {
    auto guard = ConnectionPool::instance(DatabaseType::MySQL).get_connection();
    
    if (!guard) return;

    // 准备预处理语句
    guard->prepare_statement("INSERT INTO users (id, name) VALUES (?, ?);");
    
    // 绑定参数
    guard->set_int(1, id);
    guard->set_string(2, name);
    
    // 执行
    guard->execute_update();
}
```

### 4. 事务操作

```cpp
#include "conn-pool/ConnectionPool.h"

void transfer_money(int from_id, int to_id, double amount) {
    auto guard = ConnectionPool::instance(DatabaseType::MySQL).get_connection();
    
    if (!guard) return;

    try {
        // 开始事务
        guard->begin();

        // 扣减转出账户
        guard->execute_update("UPDATE accounts SET balance = balance - " + 
                             std::to_string(amount) + " WHERE id = " + std::to_string(from_id));

        // 增加转入账户
        guard->execute_update("UPDATE accounts SET balance = balance + " + 
                             std::to_string(amount) + " WHERE id = " + std::to_string(to_id));

        // 提交事务
        guard->commit();
    } catch (...) {
        // 回滚事务
        guard->rollback();
        throw;
    }
}
```

### 5. 获取统计信息（待完善）

> **注意**: 当前 `get_statistics()` 方法返回空结构，统计信息采集功能尚未实现。

```cpp
#include "conn-pool/ConnectionPool.h"
#include "monitor/PoolStatistics.h"

void print_stats() {
    auto& pool = ConnectionPool::instance(DatabaseType::MySQL);
    PoolStatistics stats = pool.get_statistics();

    // 当前返回空结构，统计信息采集待实现
    std::cout << "Statistics feature is not implemented yet." << std::endl;
}
```

### 6. 最佳实践

```cpp
// 推荐：使用移动语义传递连接守卫
void process_data(PooledConnGuard&& guard) {
    if (!guard) {
        throw std::runtime_error("No connection available");
    }
    // 使用guard...
}

// 推荐：在函数作用域内使用连接，确保及时归还
void query_data() {
    auto guard = ConnectionPool::instance(DatabaseType::MySQL).get_connection();
    if (!guard) return;
    
    IResultSet* res = guard->execute_query("SELECT * FROM table;");
    // 处理结果...
    delete res;  // 及时释放结果集
    // guard自动归还连接
}

// 不推荐：长期持有连接
void bad_practice() {
    auto guard = ConnectionPool::instance(DatabaseType::MySQL).get_connection();
    // ... 长时间操作 ...
    // guard在函数结束时才归还，期间其他线程无法使用该连接
}
```

---

## 性能优化策略

### 1. 连接池大小配置

```cpp
// 根据业务场景调整连接池大小
auto& pool = ConnectionPool::instance(DatabaseType::MySQL);

// CPU密集型应用：较小的连接池
pool.init(cfg, 4, 8, 180, 10000, 2);

// IO密集型应用：较大的连接池
pool.init(cfg, 10, 50, 180, 10000, 10);

// 在线调整（注意：非线程安全，应在主线程中配置）
pool.set_min_size(10);
pool.set_max_size(100);
```

### 2. 连接复用策略

- **尽量短时间持有连接**: 完成操作后立即归还，避免长时间占用
- **使用RAII**: `PooledConnGuard` 确保连接自动归还
- **避免嵌套使用**: 同一线程不要同时持有多个连接

### 3. 超时配置

```cpp
// 调整获取连接超时时间（毫秒）
pool.set_timeout(30000);  // 30秒超时

// 调整连接最大空闲时间（秒）—— 尚未实现回收逻辑
pool.set_max_idle_time(600);  // 10分钟
```

### 4. 预处理语句优化

```cpp
// 推荐：使用预处理语句，避免重复解析SQL
guard->prepare_statement("SELECT * FROM users WHERE id = ?");
for (int id : user_ids) {
    guard->set_int(1, id);
    IResultSet* res = guard->execute_query();
    // 处理结果...
    delete res;
}
```

---

## 常见问题解决方案

### Q1: 获取连接超时怎么办？

**原因分析**:
- 连接池达到最大连接数，所有连接都在使用中
- 数据库响应慢，连接创建耗时过长
- 连接泄漏，未及时归还连接

**解决方案**:
```cpp
// 1. 增加最大连接数
pool.set_max_size(50);

// 2. 增加超时时间
pool.set_timeout(30000);

// 3. 检查是否存在连接泄漏（确保使用PooledConnGuard）
auto guard = pool.get_connection();
if (guard) {
    // 使用连接...
    // guard析构时自动归还
}

// 4. 增加每次扩容数量
// 在init时设置：pool.init(cfg, 5, 50, 180, 30000, 10);
```

### Q2: 连接无效或断开怎么办？

**原因分析**:
- 数据库服务重启或网络中断
- 连接长时间空闲被数据库端关闭

**解决方案**:
```cpp
// 1. 在使用前检查连接有效性
auto guard = pool.get_connection();
if (guard && guard->is_valid()) {
    // 使用连接...
}

// 2. 归还连接时会自动检查is_valid()，无效连接会被销毁
// 健康检查器集成到连接池主流程待实现

// 3. 设置合理的最大空闲时间（回收逻辑待实现）
pool.set_max_idle_time(180);  // 3分钟
```

### Q3: 如何处理高并发场景？

**解决方案**:
```cpp
// 1. 配置合适的连接池大小
pool.init(cfg, 20, 100, 180, 30000, 20);

// 2. 使用异步处理
std::vector<std::future<void>> futures;
for (int i = 0; i < 100; i++) {
    futures.push_back(std::async(std::launch::async, [&]() {
        auto guard = pool.get_connection();
        if (guard) {
            // 执行操作...
        }
    }));
}

// 3. 监控连接池状态（统计功能待实现）
// PoolStatistics stats = pool.get_statistics();
```

---

## 扩展开发指南

### 1. 添加新数据库类型支持

#### 步骤1: 创建连接实现类

```cpp
// include/database/OracleConnection.h
#pragma once
#include "IDatabaseConnection.h"

class OracleConnection : public IDatabaseConnection {
public:
    OracleConnection(const ConnectionConfig& cfg);
    ~OracleConnection() override;
    
    // 实现所有IDatabaseConnection接口方法
    OracleConnection* connection() override;
    void close() override;
    bool is_valid() override;
    bool execute(const std::string& sql) override;
    int execute_update(const std::string& sql) override;
    int execute_update() override;
    IResultSet* execute_query(const std::string& sql) override;
    IResultSet* execute_query() override;
    void prepare_statement(const std::string& sql) override;
    void begin() override;
    void commit() override;
    void rollback() override;
    uint64_t last_insert_id() override;
    
    // 实现所有参数绑定方法
    void set_int(uint16_t idx, int val) override;
    void set_string(uint16_t idx, const std::string& val) override;
    // ... 其他参数类型
};
```

#### 步骤2: 创建工厂类

```cpp
// include/database/OracleFactory.h
#pragma once
#include "IConnectionFactory.h"
#include "OracleConnection.h"

class OracleFactory : public IConnectionFactory {
public:
    ~OracleFactory() override = default;
    
    OracleConnection* create_connection(const ConnectionConfig& cfg) override {
        return new OracleConnection(cfg);
    }
    
    DatabaseType get_database_type() const override {
        return DatabaseType::Oracle;
    }
};
```

#### 步骤3: 创建结果集实现类

```cpp
// include/database/OracleResultSet.h
#pragma once
#include "IResultSet.h"

class OracleResultSet : public IResultSet {
public:
    // 实现所有IResultSet接口方法
    bool next() override;
    void close() override;
    std::size_t get_column_count() override;
    // ... 其他读取方法
};
```

#### 步骤4: 创建健康检查类

```cpp
// include/monitor/OracleHealthChecker.h
#pragma once
#include "IHealthChecker.h"

class OracleHealthChecker : public IHealthChecker {
public:
    HealthStatus check(IDatabaseConnection* conn) override;
    void set_timeout_ms(int timeout_ms) override;
};
```

#### 步骤5: 在连接池中注册

```cpp
// src/ConnectionPool.cpp - 构造函数
ConnectionPool::ConnectionPool(DatabaseType type) {
    switch (type) {
        case DatabaseType::MySQL:
            m_factory = new MySQLFactory();
            break;
        case DatabaseType::Oracle:
            m_factory = new OracleFactory();  // 添加Oracle工厂
            break;
        // ... 其他数据库类型
    }
}
```

### 2. 自定义连接池策略

#### 实现自定义扩容策略

```cpp
// 自定义连接池配置
auto& pool = ConnectionPool::instance(DatabaseType::MySQL);

// 初始化时设置参数
pool.init(cfg, 10, 100, 180, 30000, 20);

// 运行时调整
pool.set_min_size(20);
pool.set_max_size(200);
pool.set_timeout(60000);
pool.set_max_idle_time(300);
```

#### 实现自定义健康检查

```cpp
class CustomHealthChecker : public IHealthChecker {
public:
    HealthStatus check(IDatabaseConnection* conn) override {
        // 自定义检查逻辑
        // 例如：执行特定的健康检查SQL
        // 检查响应时间
        // 检查数据库负载等
        return HealthStatus::Healthy;
    }
    
    void set_timeout_ms(int timeout_ms) override {
        m_timeout_ms = timeout_ms;
    }
    
private:
    int m_timeout_ms = 5000;
};
```

### 3. 待实现功能扩展指南

以下功能尚未实现，欢迎贡献代码：

#### 功能1: 统计信息采集

**实现思路**:
1. 在 `ConnectionPool` 类中添加统计计数器成员变量
2. 在 `get_connection()` 和 `return_connection()` 方法中更新统计数据
3. 实现 `get_statistics()` 方法返回完整的统计信息

#### 功能2: 健康检查集成

**实现思路**:
1. 在 `ConnectionPool` 中启用 `m_health_checker` 成员
2. 在连接归还时调用健康检查
3. 实现定期健康检查线程，检测空闲连接的有效性

#### 功能3: 连接回收机制

**实现思路**:
1. 记录每个连接的创建时间和最后使用时间
2. 实现后台线程定期检查空闲连接
3. 对超过最大空闲时间的连接进行回收

#### 功能4: 监控回调机制

**实现思路**:
1. 在 `ConnectionPool` 中添加回调函数列表
2. 在关键事件（连接获取、归还、创建、销毁）发生时触发回调
3. 支持外部监控系统接入

*最后更新: 2026年7月3日*
