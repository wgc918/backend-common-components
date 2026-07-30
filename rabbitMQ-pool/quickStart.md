# RabbitMQ C 客户端 (librabbitmq) 快速开始笔记

本笔记基于 [librabbitmq](https://github.com/alanxz/rabbitmq-c) 的 AMQP 0-9-1 协议实现，涵盖连接、认证、交换机/队列声明、消息发布与消费等核心操作。所有接口均为同步阻塞模式，便于初学者理解。

---

## 1. 环境准备

- **头文件**：`#include <amqp.h>` 和 `#include <amqp_tcp_socket.h>`
- **链接库**：`-lrabbitmq`（编译时添加）
- **命名空间**：使用标准 C++ 库（`<iostream>`, `<string>` 等）

---

## 2. 连接与认证

### 2.1 创建连接状态
```c
amqp_connection_state_t conn = amqp_new_connection();
```
- 分配并初始化连接状态对象，后续所有操作都依赖它。

### 2.2 创建 TCP 套接字
```c
amqp_socket_t *socket = amqp_tcp_socket_new(conn);
```
- 绑定 TCP 套接字到连接状态，但尚未打开。

### 2.3 打开 TCP 连接
```c
int status = amqp_socket_open(socket, hostname.c_str(), port);
```
- 参数：`hostname`（服务器地址），`port`（通常 5672）。
- 返回值：`0` 成功，非 `0` 失败。

### 2.4 登录（认证）
```c
amqp_rpc_reply_t reply = amqp_login(conn, vhost, channel_max, frame_max, heartbeat,
                                     sasl_method, username, password);
```
- 参数：
  - `vhost`：虚拟主机名，如 `"/"`。
  - `channel_max`：最大信道数，`0` 表示使用服务器默认。
  - `frame_max`：最大帧大小，`0` 或具体值（如 131072）。
  - `heartbeat`：心跳间隔（秒），`0` 禁用。
  - `sasl_method`：认证机制，常用 `AMQP_SASL_METHOD_PLAIN`。
  - 后续可变参数：用户名和密码（字符串）。
- 返回值：`amqp_rpc_reply_t`，需检查 `reply_type` 是否为 `AMQP_RESPONSE_NORMAL`。

### 2.5 打开信道
```c
amqp_channel_open(conn, 1);                // 异步发送 Channel.Open
die_on_error(amqp_get_rpc_reply(conn), "Opening channel");
```
- `amqp_channel_open`：请求打开指定 ID（1~65535）的信道，不等待响应。
- `amqp_get_rpc_reply`：同步等待服务器的响应，返回 `amqp_rpc_reply_t`。

---

## 3. 声明交换机和队列

### 3.1 声明交换机
```c
amqp_exchange_declare(conn, channel, exchange_name, type, passive, durable,
                       auto_delete, internal, arguments);
```
- 参数（常用）：
  - `exchange_name`：交换机名称（`amqp_cstring_bytes` 转换）。
  - `type`：类型，如 `"direct"`, `"fanout"`, `"topic"`, `"headers"`。
  - `passive`：`0` 表示不存在则创建，`1` 仅检查存在性。
  - `durable`：`1` 表示持久化（重启后保留）。
  - `auto_delete`：`1` 表示无绑定后自动删除。
  - `internal`：`1` 表示仅内部使用。
  - `arguments`：扩展参数表，通常用 `amqp_empty_table`。
- 同步调用，需检查 `amqp_get_rpc_reply(conn)`。

### 3.2 声明队列
```c
amqp_queue_declare_ok_t *q = amqp_queue_declare(conn, channel, queue_name,
                                                 passive, durable, exclusive, auto_delete,
                                                 arguments);
```
- 参数：
  - `queue_name`：队列名，若为空字符串则服务器生成唯一名称。
  - `passive`：同交换机。
  - `durable`：同交换机。
  - `exclusive`：`1` 表示仅当前连接可用，连接断开后自动删除。
  - `auto_delete`：同交换机。
  - `arguments`：扩展参数（如 TTL、死信等）。
- 返回值：`amqp_queue_declare_ok_t*`，包含服务器返回的队列名、消息数、消费者数等信息（可忽略）。
- 需同步检查 `amqp_get_rpc_reply(conn)`。

### 3.3 绑定队列到交换机
```c
amqp_queue_bind(conn, channel, queue_name, exchange_name, routing_key, arguments);
```
- 参数：队列名、交换机名、路由键、额外参数表。
- 同步，需检查 `amqp_get_rpc_reply(conn)`。

---

## 4. 发布消息

### 4.1 准备消息属性（可选）
```c
amqp_basic_properties_t props;
props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
props.content_type = amqp_cstring_bytes("text/plain");
props.delivery_mode = 2;   // 2=持久化，1=非持久化
```

### 4.2 发送消息
```c
int result = amqp_basic_publish(conn, channel, exchange, routing_key,
                                mandatory, immediate, &props, body);
```
- 参数：
  - `exchange` / `routing_key`：目标交换机及路由键。
  - `mandatory`：若消息无法路由，是否返回 `Basic.Return`（需设置回调）。
  - `immediate`：已废弃，通常置 0。
  - `props`：指向属性结构的指针，可为 `NULL`。
  - `body`：消息体（`amqp_bytes_t`）。
- 返回值：`AMQP_STATUS_OK` 表示库内发送成功，**不保证** broker 已接收。
- 注意：此函数是**异步**的，如需确认请使用 Publisher Confirms 机制。

---

## 5. 消费消息

### 5.1 启动消费
```c
amqp_basic_consume(conn, channel, queue, consumer_tag, no_local, no_ack,
                   exclusive, arguments);
```
- 参数：
  - `queue`：要消费的队列名。
  - `consumer_tag`：消费者标签，可为空字符串（服务器生成）。
  - `no_local`：`1` 表示不接收自己发布的消息（仅适用于同一个连接）。
  - `no_ack`：`1` 表示自动确认，`0` 需手动确认（`amqp_basic_ack`）。
  - `exclusive`：`1` 表示独占队列。
  - `arguments`：扩展参数（如预取计数，需通过 `amqp_basic_qos` 设置）。
- 同步调用，需检查 `amqp_get_rpc_reply(conn)`。

### 5.2 接收消息循环
```c
amqp_maybe_release_buffers(conn);   // 释放已处理帧的缓冲区，防止内存增长
amqp_rpc_reply_t res = amqp_consume_message(conn, &envelope, NULL, 0);
```
- `amqp_consume_message`：阻塞等待一条消息，返回 `amqp_rpc_reply_t`。
  - 参数：连接状态、指向 `amqp_envelope_t` 的指针、超时（`NULL` 表示无限等待）、标志。
  - 成功时 `res.reply_type == AMQP_RESPONSE_NORMAL`，消息内容在 `envelope.message.body` 中。
  - 若失败（如连接断开），则跳出循环。
- 处理完消息后调用 `amqp_destroy_envelope(&envelope)` 释放资源。

### 5.3 手动确认（若 `no_ack=0`）
```c
amqp_basic_ack(conn, channel, delivery_tag, multiple);
```
- `delivery_tag` 从 `envelope.delivery_tag` 获取。
- `multiple`：`1` 表示确认该 tag 及之前所有未确认消息。

---

## 6. 清理资源

### 6.1 关闭信道
```c
amqp_channel_close(conn, channel, AMQP_REPLY_SUCCESS);
```

### 6.2 关闭连接
```c
amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
```

### 6.3 销毁连接状态
```c
amqp_destroy_connection(conn);
```

---

## 7. 错误处理辅助函数

封装一个通用检查函数：
```c
void die_on_error(amqp_rpc_reply_t x, const char *context) {
    if (x.reply_type != AMQP_RESPONSE_NORMAL) {
        std::cerr << "Error in " << context << ": "
                  << amqp_error_string2(x.library_error) << std::endl;
        exit(1);
    }
}
```
- `amqp_error_string2` 将库错误码转为可读字符串。
- 对于 `amqp_basic_publish` 等异步调用，需使用其他方式检查（如返回码）。

---

## 8. 完整示例代码

### 发送端（Publisher）
```cpp
#include <amqp.h>
#include <amqp_tcp_socket.h>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

void die_on_error(amqp_rpc_reply_t x, const char* context) { /* 同上 */ }

int main() {
    const std::string hostname = "localhost";
    const int port = 5672;
    const std::string exchange = "example_exchange";
    const std::string routing_key = "example_key";

    amqp_connection_state_t conn = amqp_new_connection();
    amqp_socket_t* socket = amqp_tcp_socket_new(conn);
    if (!socket) return 1;
    if (amqp_socket_open(socket, hostname.c_str(), port)) return 1;

    die_on_error(amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest", "guest"),
                 "Logging in");
    amqp_channel_open(conn, 1);
    die_on_error(amqp_get_rpc_reply(conn), "Opening channel");

    amqp_exchange_declare(conn, 1, amqp_cstring_bytes(exchange.c_str()),
                          amqp_cstring_bytes("direct"), 0, 0, 0, 0, amqp_empty_table);
    die_on_error(amqp_get_rpc_reply(conn), "Declaring exchange");

    for (int i = 1; i <= 10; ++i) {
        std::string msg = "Message " + std::to_string(i);
        amqp_basic_properties_t props;
        props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
        props.content_type = amqp_cstring_bytes("text/plain");
        props.delivery_mode = 2;

        int res = amqp_basic_publish(conn, 1, amqp_cstring_bytes(exchange.c_str()),
                                     amqp_cstring_bytes(routing_key.c_str()),
                                     0, 0, &props, amqp_cstring_bytes(msg.c_str()));
        if (res == AMQP_STATUS_OK)
            std::cout << "Published: " << msg << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
    amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
    amqp_destroy_connection(conn);
    return 0;
}
```

### 接收端（Consumer）
```cpp
#include <amqp.h>
#include <amqp_tcp_socket.h>
#include <iostream>
#include <string>

void die_on_error(amqp_rpc_reply_t x, const char* context) { /* 同上 */ }

int main() {
    const std::string hostname = "localhost";
    const int port = 5672;
    const std::string queue = "example_queue";
    const std::string exchange = "example_exchange";
    const std::string routing_key = "example_key";

    amqp_connection_state_t conn = amqp_new_connection();
    amqp_socket_t* socket = amqp_tcp_socket_new(conn);
    if (!socket) return 1;
    if (amqp_socket_open(socket, hostname.c_str(), port)) return 1;

    die_on_error(amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest", "guest"),
                 "Logging in");
    amqp_channel_open(conn, 1);
    die_on_error(amqp_get_rpc_reply(conn), "Opening channel");

    amqp_exchange_declare(conn, 1, amqp_cstring_bytes(exchange.c_str()),
                          amqp_cstring_bytes("direct"), 0, 0, 0, 0, amqp_empty_table);
    die_on_error(amqp_get_rpc_reply(conn), "Declaring exchange");

    amqp_queue_declare(conn, 1, amqp_cstring_bytes(queue.c_str()), 0, 0, 0, 1, amqp_empty_table);
    die_on_error(amqp_get_rpc_reply(conn), "Declaring queue");

    amqp_queue_bind(conn, 1, amqp_cstring_bytes(queue.c_str()),
                    amqp_cstring_bytes(exchange.c_str()),
                    amqp_cstring_bytes(routing_key.c_str()), amqp_empty_table);
    die_on_error(amqp_get_rpc_reply(conn), "Binding queue");

    amqp_basic_consume(conn, 1, amqp_cstring_bytes(queue.c_str()),
                       amqp_empty_bytes, 0, 1, 0, amqp_empty_table);
    die_on_error(amqp_get_rpc_reply(conn), "Consuming");

    while (true) {
        amqp_maybe_release_buffers(conn);
        amqp_envelope_t envelope;
        amqp_rpc_reply_t res = amqp_consume_message(conn, &envelope, NULL, 0);
        if (res.reply_type == AMQP_RESPONSE_NORMAL) {
            std::cout << "Received: "
                      << std::string((char*)envelope.message.body.bytes,
                                     envelope.message.body.len)
                      << std::endl;
            amqp_destroy_envelope(&envelope);
        } else {
            break;
        }
    }

    amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
    amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
    amqp_destroy_connection(conn);
    return 0;
}
```

---

## 9. 重要注意事项

- **同步 vs 异步**：`amqp_basic_publish` 是异步的，不返回服务器确认；如需可靠投递，需结合 `amqp_confirm_select` 和 `amqp_basic_ack`（Publisher Confirms）。
- **内存管理**：`amqp_maybe_release_buffers` 在消费循环中定期调用，防止缓冲区无限增长。
- **超时设置**：`amqp_consume_message` 可设置超时（第 3 个参数为 `struct timeval*`），避免永久阻塞。
- **线程安全**：librabbitmq 不支持多线程共享同一连接，每个线程应使用独立连接。
- **认证**：本例使用明文密码，生产环境建议使用 TLS（`amqp_ssl_socket_new`）或更安全的认证方式。
- **队列持久化**：如需持久化，将 `durable` 设为 `1`，并确保 `delivery_mode=2`。
- **手动确认**：若设置 `no_ack=0`，必须调用 `amqp_basic_ack` 或 `amqp_basic_nack`，否则消息会一直积压。

---

## 10. 常用扩展配置

- **预取计数**（限制未确认消息数）：
  ```c
  amqp_basic_qos(conn, channel, 0, prefetch_count, 0);
  ```
- **死信交换机**：在 `amqp_queue_declare` 的 `arguments` 中设置 `"x-dead-letter-exchange"` 等。


## 11. 参考博客
[rabbitmq 架构1](https://blog.csdn.net/hezuijiudexiaobai/article/details/147949264)
[rabbitmq 架构2](https://developer.aliyun.com/article/1409948)

---

本笔记涵盖了 RabbitMQ C 客户端最核心的 API 使用，可快速搭建生产者和消费者原型。更多细节请参阅 [官方文档](https://rabbitmq.github.io/rabbitmq-c/) 和 AMQP 0-9-1 规范。