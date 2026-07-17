# Redis Connection Pool

高性能、线程安全的 C++ Redis 连接池库，基于 `redis-plus-plus` 封装，提供简洁的 API 接口和自动化的连接生命周期管理。

## 目录

- [项目概述](#项目概述)
- [核心功能特性](#核心功能特性)
- [架构设计](#架构设计)
- [快速开始](#快速开始)
- [API 参考](#api-参考)
- [配置参数说明](#配置参数说明)
- [使用示例](#使用示例)
- [最佳实践](#最佳实践)

---

## 项目概述

本项目是一个基于 C++17 开发的 Redis 连接池库，采用现代化的设计模式，旨在为后端服务提供高效、可靠的 Redis 连接管理能力。框架遵循面向对象设计原则，通过 RAII 守卫自动管理连接生命周期，避免连接泄漏。

### 设计目标

1. **高性能**: 预创建连接，最小化连接建立开销
2. **线程安全**: 完善的并发控制机制，支持多线程环境
3. **自动重连**: 连接断开后自动检测并重建
4. **资源回收**: 后台线程定期回收空闲连接，避免资源浪费
5. **易于使用**: 简洁的 API，RAII 自动资源管理
6. **可观测性**: 内置统计信息，便于监控和调优

---

## 核心功能特性

| 特性 | 描述 |
|------|------|
| **单例模式** | 全局唯一连接池实例，避免资源浪费 |
| **连接池自动扩容** | 根据实际需求动态调整连接数量 |
| **连接超时机制** | 获取连接支持超时配置，避免线程无限等待 |
| **RAII 连接管理** | 通过 `RedisConnGuard` 自动归还连接，防止连接泄漏 |
| **自动重连** | 归还连接时检测有效性，无效连接自动销毁并重建 |
| **空闲连接回收** | 后台线程定期回收超时空闲连接 |
| **健康检查** | 基于 PING 命令的连接有效性检测 |
| **统计信息** | 提供借出/归还/创建/销毁等运行指标 |
| **线程安全** | 使用互斥锁和条件变量保证并发安全 |

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────┐
│                   应用层 (Application Layer)                  │
│           RedisConnGuard → 业务逻辑 (SET/GET/...)             │
└──────────────────────┬──────────────────────────────────────┘
                       │ 获取连接 / 归还连接
                       ▼
┌─────────────────────────────────────────────────────────────┐
│                 连接池层 (Connection Pool Layer)              │
│  ┌─────────────────────────────────────────────────────────┐│
│  │                RedisConnectionPool                      ││
│  │  ┌──────────┐  ┌──────────┐  ┌───────────────────────┐ ││
│  │  │ 队列管理 │  │ 扩容策略 │  │  统计与监控模块        │ ││
│  │  └────┬─────┘  └────┬─────┘  └───────────────────────┘ ││
│  │       │             │                                   ││
│  │       ▼             ▼                                   ││
│  │  ┌───────────────────────────────────────────────────┐  ││
│  │  │  std::queue<pair<Redis*, TimePoint>>              │  ││
│  │  └───────────────────────────────────────────────────┘  ││
│  └─────────────────────────────────────────────────────────┘│
│  ┌──────────────┐  ┌──────────────────┐                     │
│  │ 回收线程      │  │  健康检查器       │                     │
│  └──────────────┘  └──────────────────┘                     │
└──────────────────────┬──────────────────────────────────────┘
                       │ 创建连接
                       ▼
┌─────────────────────────────────────────────────────────────┐
│               连接工厂层 (Connection Factory Layer)           │
│              RedisConnectionFactory                          │
└──────────────────────┬──────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────┐
│                  底层库 (Driver Layer)                        │
│        redis-plus-plus (sw::redis::Redis)                    │
│                   hiredis (C client)                         │
└─────────────────────────────────────────────────────────────┘
```

### 模块说明

| 文件 | 职责 |
|------|------|
| `RedisConnectionPool.h/cpp` | 连接池核心类，管理连接队列、线程同步、扩容策略、回收线程 |
| `RedisConnGuard.h/cpp` | RAII 连接守卫，自动归还连接 |
| `RedisConnectionFactory.h/cpp` | 连接工厂，根据配置创建 Redis 连接 |
| `RedisHealthChecker.h/cpp` | 健康检查器，PING 命令检测连接有效性 |
| `PoolStatistics.h` | 统计信息结构 |

---

## 快速开始

### 环境要求

- **操作系统**: Linux / macOS
- **编译器**: GCC 7+ / Clang 6+
- **CMake**: 3.16+
- **依赖**: redis-plus-plus, hiredis（环境搭建参照 [install.md](install.md)）

### 编译

```bash
# 在 redis-connection-pool 目录下
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### 最简单使用

```cpp
#include "RedisConnectionPool.h"
#include "RedisConnGuard.h"

int main() {
    // 1. 配置连接参数
    RedisConnectionConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = 6379;

    // 2. 初始化连接池
    RedisConnectionPool::instance().init(cfg);

    // 3. 获取连接并使用
    auto guard = RedisConnectionPool::instance().get_connection();
    if (guard) {
        guard->set("key", "value");
        auto val = guard->get("key");
        if (val) {
            std::cout << *val << std::endl;
        }
    }
    // guard 析构时自动归还连接

    // 4. 关闭连接池
    RedisConnectionPool::instance().shutdown();
    return 0;
}
```

---

## API 参考

### RedisConnectionPool

连接池核心类，全局单例。

#### 单例管理

```cpp
// 获取连接池单例
static RedisConnectionPool& instance();

// 销毁连接池单例
static void destroy_instance();
```

#### 初始化

```cpp
// 使用默认参数初始化
void init(const RedisConnectionConfig& cfg);

// 使用自定义参数初始化
void init(const RedisConnectionConfig& cfg, int min, int max,
          int idle_time, int timeout, int op_num);
```

#### 连接获取

```cpp
// 从连接池获取连接，返回 RAII 守卫
// 获取失败时 operator bool() 返回 false
RedisConnGuard get_connection();
```

#### 参数配置（非线程安全）

```cpp
void set_min_size(int val);
void set_max_size(int val);
void set_max_idle_time(int val);
void set_timeout(int val);

int get_min_size() const;
int get_max_size() const;
int get_max_idle_time() const;
int get_timeout() const;
```

#### 统计信息

```cpp
PoolStatistics get_statistics() const;
```

#### 生命周期

```cpp
void shutdown();
```

### RedisConnGuard

RAII 连接守卫，自动归还连接。

```cpp
// 成员访问（直接调用 sw::redis::Redis 的方法）
sw::redis::Redis* operator->() const;

// 解引用
sw::redis::Redis& operator*() const;

// 获取原始指针
sw::redis::Redis* get() const;

// 有效性检查
explicit operator bool() const;

// 主动归还
void release();
```

### RedisConnectionConfig

```cpp
struct RedisConnectionConfig {
    std::string host = "127.0.0.1";  // Redis 服务器地址
    int port         = 6379;          // Redis 端口
    std::string password;             // 密码（默认为空）
    int db           = 0;             // 数据库编号
    int connect_timeout = 1000;       // 连接超时（毫秒）
    int socket_timeout  = 1000;       // socket 读写超时（毫秒）
    bool keep_alive     = true;       // 是否启用 TCP keepalive
};
```

### PoolStatistics

```cpp
struct PoolStatistics {
    int active_connections;      // 当前活跃连接数
    int idle_connections;        // 当前空闲连接数
    int total_connections;       // 当前连接总数

    uint64_t total_borrowed;         // 累计借出次数
    uint64_t total_released;         // 累计归还次数
    uint64_t total_created;          // 累计创建连接数
    uint64_t total_destroyed;        // 累计销毁连接数
    uint64_t total_timeout_errors;   // 累计超时错误次数
    uint64_t total_reconnect_count;  // 累计重连次数

    int pending_waiters;             // 当前等待线程数
    int64_t avg_wait_time_ms;        // 平均等待时长
    int64_t max_wait_time_ms;        // 最大等待时长
};
```

---

## 配置参数说明

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `min` | CPU 核心数 | 连接池最小连接数，初始化时创建 |
| `max` | min × 2 | 连接池最大连接数，达到后不再扩容 |
| `idle_time` | 180 秒 | 连接最大空闲时间，超过后由回收线程回收 |
| `timeout` | 5000 毫秒 | 获取连接超时时间，超时返回空守卫 |
| `op_num` | 4 | 每次扩容的连接数量 |

---

## 使用示例

### 1. 基础使用

```cpp
#include "RedisConnectionPool.h"
#include "RedisConnGuard.h"

int main() {
    RedisConnectionConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = 6379;

    // 自定义参数初始化
    auto& pool = RedisConnectionPool::instance();
    pool.init(cfg, 4, 20, 180, 10000, 4);

    auto guard = pool.get_connection();
    if (guard) {
        guard->set("hello", "world");
        auto val = guard->get("hello");
        if (val) std::cout << *val << std::endl;
    }
    // guard 自动归还

    pool.shutdown();
    return 0;
}
```

### 2. 多线程并发

```cpp
void worker(int id) {
    auto guard = RedisConnectionPool::instance().get_connection();
    if (!guard) return;

    guard->set("thread_" + std::to_string(id), std::to_string(id));
    // guard 自动归还
}

int main() {
    // ... 初始化连接池 ...

    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.emplace_back(worker, i);
    }
    for (auto& t : threads) t.join();

    // ... 关闭连接池 ...
}
```

### 3. 获取统计信息

```cpp
PoolStatistics stats = RedisConnectionPool::instance().get_statistics();
std::cout << "Active: " << stats.active_connections
          << ", Idle: " << stats.idle_connections
          << ", Total: " << stats.total_connections << std::endl;
std::cout << "Borrowed: " << stats.total_borrowed
          << ", Released: " << stats.total_released << std::endl;
```

### 4. 移动语义传递

```cpp
void process(RedisConnGuard&& guard) {
    if (guard) {
        guard->set("key", "value");
    }
}

void caller() {
    auto guard = RedisConnectionPool::instance().get_connection();
    process(std::move(guard));
    // guard 已被移动，不再持有连接
}
```

---

## 最佳实践

### ✅ 推荐做法

1. **短时间持有连接**: 完成操作后让 guard 尽快析构，避免长期占用
2. **使用 RAII**: 始终通过 `RedisConnGuard` 获取连接，确保自动归还
3. **检查有效性**: 使用前检查 `if (guard)` 确保获取成功
4. **主线程配置**: 初始化、参数调整、关闭操作统一在主线程执行
5. **合理配置大小**: CPU 密集型应用用较小连接池，IO 密集型适当增大

### ❌ 避免做法

1. **长期持有连接**: 不要在 guard 作用域内执行长时间非 Redis 操作
2. **手动管理连接**: 不要直接 `new`/`delete` `sw::redis::Redis*`，始终通过连接池
3. **嵌套获取**: 同一线程不要同时持有多个 guard
4. **忽略返回值**: 始终检查 `get_connection()` 返回值是否有效

---

*最后更新: 2026年7月*
