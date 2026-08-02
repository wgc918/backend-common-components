# rabbitMQ-pool

<div align="center">


**基于 rabbitmq-c 的高性能 C++ RabbitMQ 客户端连接池**

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)

</div>

---

## 📖 目录

- [项目简介](#-项目简介)
- [架构设计](#-架构设计)
- [功能特性](#-功能特性)
- [快速开始](#-快速开始)
- [配置说明](#-配置说明)
- [核心概念](#-核心概念)
- [API 文档](#-api-文档)
- [使用示例](#-使用示例)
- [RabbitMQ 工作模式](#-rabbitmq-工作模式)
- [常见问题](#-常见问题)
- [贡献指南](#-贡献指南)
- [许可证](#-许可证)

---

## 📖 项目简介

`rabbitMQ-pool` 是一个基于 [rabbitmq-c](https://github.com/alanxz/rabbitmq-c) 库的高性能、线程安全的 C++ RabbitMQ 客户端封装库。它通过 **Channel 对象池** 的设计模式，实现了 AMQP Channel 的高效复用，避免了频繁创建/销毁 Channel 带来的性能开销。



### 设计理念

- **池化复用**：预创建 Channel 并统一管理，借出/归还机制保证线程安全
- **RAII 保障**：ChannelGuard 自动归还、Connection 自动释放，杜绝资源泄漏
- **链式 API**：流畅的链式调用风格，代码简洁易读
- **分层清晰**：Connection → Channel → Client 三层架构，职责分明

---

## 🏗️ 架构设计

```
┌─────────────────────────────────────────────────────────────┐
│                        应用层 (Application)                   │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐  │
│  │   Producer   │  │   Consumer   │  │     Broker       │  │
│  │  (消息发送)   │  │  (消息消费)   │  │  (拓扑编排器)     │  │
│  └──────┬───────┘  └──────┬───────┘  └────────┬─────────┘  │
│         │                 │                    │             │
│  ┌──────▼─────────────────▼────────────────────▼─────────┐  │
│  │                   ChannelPool                          │  │
│  │              (Channel 对象池 · 线程安全)                 │  │
│  │   ┌──────────┐  ┌──────────┐  ┌──────────────────┐    │  │
│  │   │ Channel 1 │  │ Channel 2 │  │ ... Channel N    │    │  │
│  │   └──────────┘  └──────────┘  └──────────────────┘    │  │
│  └───────────────────────┬───────────────────────────────┘  │
│                          │                                   │
│  ┌───────────────────────▼───────────────────────────────┐  │
│  │                    Connection                          │  │
│  │              (AMQP TCP 连接 · 互斥锁)                   │  │
│  └───────────────────────┬───────────────────────────────┘  │
│                          │                                   │
└──────────────────────────┼───────────────────────────────────┘
                           │
                    ┌──────▼──────┐
                    │  RabbitMQ   │
                    │   Server    │
                    └─────────────┘
```

### 分层职责

| 层级         | 组件                                       | 职责                                       |
| :----------- | :----------------------------------------- | :----------------------------------------- |
| **连接层**   | `Connection`                               | 管理 TCP 连接、认证、重连、互斥锁          |
| **信道层**   | `Channel` / `ChannelPool` / `ChannelGuard` | Channel 的创建、池化、借出/归还、AMQP 操作 |
| **客户端层** | `Producer` / `Consumer`                    | 消息发送、消费循环、回调通知               |
| **拓扑层**   | `Broker` / `Exchange` / `Queue`            | 声明交换机/队列、建立绑定关系              |
| **数据层**   | `Message`                                  | 消息体、路由信息、AMQP 属性封装            |

---

## ✨ 功能特性

### 核心能力

- ✅ **Channel 对象池**：预创建 Channel，线程安全借出/归还，支持阻塞和非阻塞超时获取
- ✅ **RAII 资源管理**：ChannelGuard 自动归还、Connection 自动释放，杜绝泄漏
- ✅ **消息发送**：支持单条发送、批量发送，支持直接传参或 Message 对象
- ✅ **消息消费**：独立线程运行消费循环，通过回调函数处理消息和错误
- ✅ **手动/自动确认**：支持 `no_ack` 自动确认和手动 `basic_ack`/`basic_nack`
- ✅ **QoS 预取控制**：`prefetch_count` 控制消费端负载，实现公平分发
- ✅ **断线重连**：`Connection::reconnect()` 一键重连
- ✅ **拓扑编排**：`Broker` 统一管理 Exchange/Queue 的声明与绑定，支持一键 setup/teardown

### 支持的交换机类型

| 类型    | 枚举值                  | 说明                              |
| :------ | :---------------------- | :-------------------------------- |
| Direct  | `ExchangeType::Direct`  | 路由键精确匹配                    |
| Fanout  | `ExchangeType::Fanout`  | 广播到所有绑定队列                |
| Topic   | `ExchangeType::Topic`   | 路由键模式匹配（支持 `*` 和 `#`） |
| Headers | `ExchangeType::Headers` | 基于消息头属性匹配                |

---

## 🚀 快速开始

### 环境要求

| 依赖            | 版本要求                                     |
| :-------------- | :------------------------------------------- |
| C++ 编译器      | 支持 C++17（GCC 8+ / Clang 7+ / MSVC 2019+） |
| CMake           | ≥ 3.16                                       |
| rabbitmq-c      | 最新稳定版                                   |
| RabbitMQ Server | 3.8+                                         |

### 安装 rabbitmq-c

详见[rabbitmq 环境搭建指南](./docs/install.md)

### 示例教程

根目录下的`examples` 中有完整的流程化示例程序，还有常用的5中工作模式的示例代码。

---

## ⚙️ 配置说明

### 连接配置 (`ConnConfig`)

```cpp
struct ConnConfig
{
    std::string host        = "127.0.0.1";  // RabbitMQ 服务器地址
    int         port        = 5672;         // 服务器端口
    std::string user        = "zsr";        // 用户名
    std::string password    = "123456";     // 密码
    std::string vhost       = "/";          // 虚拟主机
    int         channel_max = 0;            // 最大通道数（0 = 服务器默认）
    int         frame_max   = 0;            // 最大帧大小（0 = 服务器默认）
    int         heartbeat   = 0;            // 心跳间隔（0 = 禁用）
};
```

### 队列配置 (`QueueConfig`)

```cpp
struct QueueConfig
{
    std::string  name;                            // 队列名称
    bool         durable     = false;             // 是否持久化（服务器重启后保留）
    bool         exclusive   = false;             // 是否独占（仅当前连接可用）
    bool         auto_delete = true;              // 是否自动删除（无消费者时删除）
    amqp_table_t arguments   = amqp_empty_table;  // 扩展参数
};
```

### 交换机配置 (`ExchangeConfig`)

```cpp
struct ExchangeConfig
{
    std::string  name;                                // 交换机名称
    ExchangeType type        = ExchangeType::Direct;  // 交换机类型
    bool         durable     = false;                 // 是否持久化
    bool         auto_delete = false;                 // 是否自动删除
    bool         internal    = false;                 // 是否为内部交换机
    amqp_table_t arguments   = amqp_empty_table;      // 扩展参数
};
```

---

## 🧠 核心概念

### Channel 池化机制

```
                  ┌─────────────────────┐
                  │    ChannelPool       │
                  │  ┌───┐ ┌───┐ ┌───┐  │
  acquire() ──────▶  │ C1│ │ C2│ │ C3│  │ ◀────── release()
  (阻塞等待)        │  └───┘ └───┘ └───┘  │        (自动归还)
                  └─────────────────────┘
```

- **`acquire()`**：阻塞等待，直到有可用 Channel
- **`try_acquire(timeout)`**：带超时获取，超时返回空 guard
- **`ChannelGuard`**：RAII 守卫，析构时自动归还 Channel

### 消息确认模式

| 模式         | `no_ack` | 行为                                      |
| :----------- | :------- | :---------------------------------------- |
| **自动确认** | `true`   | 消息被投递后立即确认，无需手动处理        |
| **手动确认** | `false`  | 消费者需显式调用 `basic_ack`/`basic_nack` |

> **推荐**：对可靠性要求高的场景使用手动确认模式，配合 `prefetch_count=1` 实现公平分发。

---

## 📚 API 文档

### Connection

```cpp
class Connection
{
public:
    bool init(const ConnConfig& cfg);    // 初始化连接
    bool reconnect();                     // 重新连接
    bool is_connected() const;            // 检查连接状态
};
```

### ChannelPool

```cpp
class ChannelPool
{
public:
    ChannelPool(Connection& conn, int channel_max);

    ChannelGuard acquire();                                    // 阻塞获取 Channel
    ChannelGuard try_acquire(std::chrono::milliseconds timeout); // 带超时获取
    size_t available() const;                                   // 可用 Channel 数量
    size_t size() const;                                        // 池总容量
};
```

### Producer

```cpp
class Producer
{
public:
    explicit Producer(ChannelPool& pool);

    bool send(const Message& message);                                   // 发送 Message
    bool send(const std::string& exchange, const std::string& routing_key,
              const std::string& body, const amqp_basic_properties_t* props = nullptr); // 直接发送

    template <typename Iterator>
    int send_batch(Iterator begin, Iterator end);  // 批量发送
};
```

### Consumer

```cpp
class Consumer
{
public:
    explicit Consumer(ChannelPool& pool);

    // 链式配置
    Consumer& set_queue(const std::string& queue);
    Consumer& set_consumer_tag(const std::string& tag);
    Consumer& set_no_ack(bool no_ack);
    Consumer& set_prefetch_count(int count);
    Consumer& set_timeout(int timeout_ms);

    // 回调设置
    Consumer& on_message(MessageHandler handler);
    Consumer& on_error(ConsumerErrorHandler handler);

    // 生命周期
    void start();          // 启动消费线程
    void stop();           // 请求停止
    void join();           // 等待线程结束
    bool is_running() const;
};
```

### Broker

```cpp
class Broker
{
public:
    Broker& add_exchange(ExchangeConfig cfg);
    Broker& add_queue(QueueConfig cfg, std::vector<Binding> bindings = {});

    bool setup(Channel& channel);      // 一键声明所有拓扑
    bool teardown(Channel& channel);   // 一键拆除所有拓扑
};
```

### Message

```cpp
class Message
{
public:
    Message(std::string body, std::string exchange = "", std::string routing_key = "");

    // 链式设置属性
    Message& set_body(std::string body);
    Message& set_exchange(std::string exchange);
    Message& set_routing_key(std::string routing_key);
    Message& set_content_type(std::string type);
    Message& set_content_encoding(std::string encoding);
    Message& set_delivery_mode(uint8_t mode);   // 1=非持久化, 2=持久化
    Message& set_correlation_id(std::string id);
    Message& set_reply_to(std::string queue);
    Message& set_message_id(std::string id);
    Message& set_priority(uint8_t priority);
    Message& set_expiration(std::string expiration);
    Message& set_headers(const amqp_table_t& headers);
};
```

---

## 🔄 RabbitMQ 工作模式

`rabbitMQ-pool` 完整支持 RabbitMQ 五种核心工作模式：

| 模式          | 交换机类型   | 核心特点                    | 示例目录                 |
| :------------ | :----------- | :-------------------------- | :----------------------- |
| **简单模式**  | （默认直连） | 一对一，单生产者 → 单消费者 | `examples/simple/`       |
| **工作队列**  | （默认直连） | 竞争消费，负载均衡          | `examples/work/`         |
| **发布/订阅** | `fanout`     | 广播到所有绑定队列          | `examples/route-fanout/` |
| **路由模式**  | `direct`     | 路由键精确匹配              | `examples/route-direct/` |
| **主题模式**  | `topic`      | 通配符模糊匹配（`*` `#`）   | `examples/route-topic/`  |

> 详细说明请参阅 [RabbitMQ_workingMode.md](./docs/RabbitMQ_workingMode.md)

---

## ❓ 常见问题

### Q1：ChannelPool 的大小应该设置为多少？

**建议**：通常设置为 **并发消费者数量 + 2~5**。例如，3 个 Consumer + 1 个 Producer，建议设置 `channel_max = 5~8`。注意 Consumer 会独占一个 Channel 不归还，所以池大小应大于 Consumer 数量。

### Q2：手动确认模式下，消息确认如何工作？

当 `set_no_ack(false)` 时，Consumer 在回调 `on_message` 执行完毕后，自动调用 `basic_ack` 确认消息。如果回调中抛出异常，消息不会被确认，RabbitMQ 会重新投递。

### Q3：如何实现消息的可靠投递？

```cpp
// 1. 持久化交换机
ExchangeConfig ex_cfg{"my_exchange", ExchangeType::Direct, true};  // durable=true

// 2. 持久化队列
QueueConfig q_cfg{"my_queue", true};  // durable=true

// 3. 持久化消息
msg.set_delivery_mode(2);  // 2 = 持久化

// 4. 手动确认 + 公平分发
consumer.set_no_ack(false).set_prefetch_count(1);
```

### Q4：Consumer 线程安全吗？

是的。`Consumer` 在独立线程中运行消费循环，通过 `start()`/`stop()`/`join()` 控制生命周期。`stop()` 是线程安全的（通过 `std::atomic<bool>` 控制），`join()` 等待线程退出。每个 Channel 通过 Connection 的互斥锁保证底层操作的线程安全。

### Q5：连接断开后如何处理？

调用 `Connection::reconnect()` 重新建立连接。注意：重连后需要重新创建 ChannelPool，因为旧的 Channel 已经失效。

### Q6：如何优雅地停止 Consumer？

```cpp
std::atomic<bool> g_running{true};

// 注册信号处理
std::signal(SIGINT, [](int) { g_running.store(false); });

// 设置超时以便及时响应退出信号
consumer.set_timeout(1000);

// 等待退出信号
while (g_running.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// 优雅退出
consumer.stop();
consumer.join();
```

### Q7：生产者和消费者可以共用一个ChannelPool吗？

是可以的。比如：当生产者和消费者属于同一个应用（进程）时，使用同一个`ChannelPool`是个不错的选择，因为整个进程与mq服务器之前只建立了一条TCP连接，资源消耗极小。但是如果消费者和生产者之间的消息传递很平凡，为提高吞吐量，可使用用不同的`ChannelPool`。

## 📄 许可证

本项目基于 **MIT License** 开源。

```
MIT License

Copyright (c) 2026 wgc

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
```

---

## 📂 项目结构

```
rabbitMQ-pool/
├── include/                     # 头文件
│   ├── broker/                  # 拓扑编排
│   │   ├── broker.h             #   Broker - 拓扑编排器
│   │   ├── exchange.h           #   Exchange - 交换机配置与操作
│   │   └── queue.h              #   Queue - 队列配置与操作
│   ├── channnel/                # Channel 管理
│   │   ├── channel.h            #   Channel - AMQP 信道封装
│   │   ├── channelGuard.h       #   ChannelGuard - RAII 守卫
│   │   └── channelPool.h        #   ChannelPool - 信道对象池
│   ├── client/                  # 客户端
│   │   ├── consumer.h           #   Consumer - 消息消费者
│   │   └── producer.h           #   Producer - 消息生产者
│   ├── connection/              # 连接管理
│   │   └── connection.h         #   Connection - AMQP 连接封装
│   ├── core/                    # 核心类型
│   │   └── types.h              #   ExchangeType 枚举
│   └── message/                 # 消息封装
│       └── message.h            #   Message - 消息体与属性
├── src/                         # 源文件
│   ├── broker.cpp
│   ├── channel.cpp
│   ├── channelGuard.cpp
│   ├── channelPool.cpp
│   ├── connection.cpp
│   ├── consumer.cpp
│   ├── exchange.cpp
│   ├── message.cpp
│   ├── producer.cpp
│   └── queue.cpp
├── examples/                    # 示例代码
│   ├── send.cpp                 #   基础发送示例
│   ├── recv.cpp                 #   基础接收示例
│   ├── simple/                  #   简单模式
│   ├── work/                    #   工作队列模式
│   ├── route-direct/            #   路由模式
│   ├── route-fanout/            #   发布/订阅模式
│   └── route-topic/             #   主题模式
├── docs      					 # 文档详细说明
└── README.md                    # 本文件
```

---

<div align="center">

**⭐ 如果这个项目对你有帮助，请给一个 Star！**

</div>