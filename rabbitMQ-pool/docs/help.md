# RabbitMQ-C (librabbitmq) 常用 API 详细文档

RabbitMQ-C 是 RabbitMQ 官方提供的 C 语言客户端库（librabbitmq）。它基于 AMQP 0-9-1 协议实现。本文档涵盖最常用的 API，并详细解释每个函数的**参数**、**返回值**、**行为**、**阻塞特性**及**使用场景**。

---

## 一、核心数据结构 (Core Data Types)
在深入 API 前，需理解几个核心类型：

| 类型 | 说明 |
| :--- | :--- |
| `amqp_connection_state_t` | 连接状态机句柄，代表一个连接实例。使用 `amqp_new_connection()` 创建。 |
| `amqp_channel_t` | 通道 ID（整数，通常从 1 开始）。 |
| `amqp_bytes_t` | 字节缓冲区结构，包含 `len` (size_t) 和 `bytes` (void*)。用于传递队列名、消息体等。 |
| `amqp_table_t` | 键值对表，用于设置参数（如队列参数、Exchange 参数）。 |
| `amqp_rpc_reply_t` | RPC 调用的标准回复结构。包含 `reply_type` (回复类型) 和 `reply` (具体数据)。用于检查操作是否成功。 |
| `amqp_envelope_t` / `amqp_message_t` | 消息结构体，包含消息体、属性（Properties）和路由信息。 |

---

## 二、连接与生命周期管理 (Connection Lifecycle)

### 1. `amqp_new_connection`
```c
amqp_connection_state_t amqp_new_connection(void);
```
*   **行为**：创建一个新的连接状态对象。此时并未建立 TCP 套接字，仅初始化内存结构。
*   **参数**：无。
*   **返回值**：新分配的连接状态句柄。
*   **同步/异步**：**同步（非阻塞）**。纯内存分配，不涉及 I/O。

---

### 2. `amqp_open_socket`
```c
int amqp_open_socket(amqp_connection_state_t state, const char *hostname, int portnumber);
```
*   **行为**：解析主机名并建立 TCP 连接。将套接字文件描述符挂载到连接状态上。
*   **参数**：
    *   `state`：连接状态句柄。
    *   `hostname`：RabbitMQ 服务器 IP 或域名（如 `"localhost"`）。
    *   `portnumber`：端口号（通常为 `5672`，TLS 为 `5671`）。
*   **返回值**：成功返回套接字描述符（`int`），失败返回负错误码。
*   **同步/异步**：**同步（阻塞）**。此函数会阻塞直到 TCP 握手完成或超时。

---

### 3. `amqp_login`
```c
amqp_rpc_reply_t amqp_login(amqp_connection_state_t state, const char *vhost, int channel_max, int frame_max, int heartbeat, const amqp_table_t *properties, amqp_sasl_method_enum sasl_method);
```
*   **行为**：执行 AMQP 协议握手（发送 `Connection.Start/Ok`、`Tune/Ok`），进行认证并切换虚拟主机。
*   **参数**：
    *   `state`：连接句柄。
    *   `vhost`：虚拟主机名称（通常为 `"/"`）。
    *   `channel_max`：最大通道数（设为 `0` 表示使用 Broker 默认值，或使用 `AMQP_DEFAULT_MAX_CHANNELS`）。
    *   `frame_max`：最大帧大小（设为 `0` 使用默认值，或 `AMQP_DEFAULT_MAX_FRAME_SIZE`）。
    *   `heartbeat`：心跳间隔（秒）。设为 `0` 禁用，建议 `60` 秒。
    *   `properties`：客户端属性表（如客户端版本），通常传 `NULL` 或 `amqp_empty_table`。
    *   `sasl_method`：认证机制。通常使用 `AMQP_SASL_METHOD_PLAIN`。
*   **返回值**：`amqp_rpc_reply_t`。必须检查 `reply_type == AMQP_RESPONSE_NORMAL` 表示登录成功。
*   **同步/异步**：**同步（阻塞）**。会阻塞直到 Broker 响应认证结果。

---

### 4. `amqp_login` 的便利宏
```c
amqp_rpc_reply_t amqp_login(..., AMQP_DEFAULT_MAX_CHANNELS, AMQP_DEFAULT_MAX_FRAME_SIZE, AMQP_DEFAULT_HEARTBEAT, ...);
```
通常配合 `amqp_socket_open` 使用。为了方便，库提供了默认参数宏。

---

### 5. `amqp_connection_close`
```c
amqp_rpc_reply_t amqp_connection_close(amqp_connection_state_t state, int code);
```
*   **行为**：发送 `Connection.Close` 方法，优雅关闭连接（通知 Broker 清理资源）。
*   **参数**：
    *   `state`：连接句柄。
    *   `code`：关闭码（通常传 `AMQP_REPLY_SUCCESS`）。
*   **返回值**：RPC 回复。
*   **同步/异步**：**同步（阻塞）**。等待 Broker 回复 `Close-Ok`。

---

### 6. `amqp_destroy_connection`
```c
void amqp_destroy_connection(amqp_connection_state_t state);
```
*   **行为**：销毁连接状态对象，释放内存并关闭底层套接字（如果尚未关闭）。
*   **参数**：连接句柄。
*   **返回值**：无。
*   **同步/异步**：**同步（非阻塞）**。内存清理，若套接字开启则执行 `close()`。

---

## 三、通道管理 (Channel Management)

### 7. `amqp_channel_open`
```c
amqp_rpc_reply_t amqp_channel_open(amqp_connection_state_t state, amqp_channel_t channel);
```
*   **行为**：在已建立的连接上开启一个新的通道（Channel）。通道是多路复用的逻辑链路。
*   **参数**：
    *   `state`：连接句柄。
    *   `channel`：通道号（通常从 1 递增，如 `1`）。
*   **返回值**：RPC 回复。
*   **同步/异步**：**同步（阻塞）**。等待 Broker 的 `Channel.Open-Ok`。

---

### 8. `amqp_channel_close`
```c
amqp_rpc_reply_t amqp_channel_close(amqp_connection_state_t state, amqp_channel_t channel, int code);
```
*   **行为**：关闭指定通道。
*   **参数**：
    *   `state`：连接句柄。
    *   `channel`：通道号。
    *   `code`：关闭码（通常 `AMQP_REPLY_SUCCESS`）。
*   **返回值**：RPC 回复。
*   **同步/异步**：**同步（阻塞）**。

---

## 四、交换机与队列声明 (Declarations)

### 9. `amqp_exchange_declare`
```c
amqp_rpc_reply_t amqp_exchange_declare(amqp_connection_state_t state, amqp_channel_t channel, 
    amqp_bytes_t exchange, amqp_bytes_t type, amqp_boolean_t passive, 
    amqp_boolean_t durable, amqp_boolean_t auto_delete, amqp_boolean_t internal, 
    const amqp_table_t *arguments);
```
*   **行为**：声明一个交换机。如果不存在则创建，若存在则校验属性是否一致。
*   **参数**：
    *   `exchange`：交换机名称（如 `amq.direct`）。
    *   `type`：类型（`"direct"`, `"fanout"`, `"topic"`, `"headers"`）。
    *   `passive`：`1` 表示仅检查是否存在（不创建），不存在则报错；`0` 表示不存在则创建。
    *   `durable`：是否持久化（重启后保留）。
    *   `auto_delete`：最后一个队列解绑后自动删除。
    *   `internal`：是否为内部交换机（仅供 Broker 内部使用）。
    *   `arguments`：附加参数（如 Alternate Exchange），传 `amqp_empty_table`。
*   **返回值**：RPC 回复。
*   **同步/异步**：**同步（阻塞）**。

---

### 10. `amqp_queue_declare`
```c
amqp_rpc_reply_t amqp_queue_declare(amqp_connection_state_t state, amqp_channel_t channel, 
    amqp_bytes_t queue, amqp_boolean_t passive, amqp_boolean_t durable, 
    amqp_boolean_t exclusive, amqp_boolean_t auto_delete, const amqp_table_t *arguments);
```
*   **行为**：声明一个队列。
*   **参数**：
    *   `queue`：队列名。若传入 `amqp_empty_bytes`，Broker 将生成一个唯一名称（如 `amq.gen-...`）。
    *   `exclusive`：排他性（仅当前连接可见，断开自动删除）。
    *   其他参数同 Exchange。
*   **返回值**：RPC 回复。成功时，`reply.reply.declared.queue`（`amqp_bytes_t`）包含队列名称。**务必保存此名称**（若为自动生成）。
*   **同步/异步**：**同步（阻塞）**。

---

### 11. `amqp_queue_bind`
```c
amqp_rpc_reply_t amqp_queue_bind(amqp_connection_state_t state, amqp_channel_t channel, 
    amqp_bytes_t queue, amqp_bytes_t exchange, amqp_bytes_t routing_key, 
    const amqp_table_t *arguments);
```
*   **行为**：将队列绑定到交换机，并指定路由键（Routing Key）。
*   **参数**：
    *   `routing_key`：路由键。对于 Fanout 交换机通常设为 `""`。
    *   `arguments`：附加绑定参数（如 Headers 匹配），传 `amqp_empty_table`。
*   **返回值**：RPC 回复。
*   **同步/异步**：**同步（阻塞）**。

---

## 五、消息发布 (Publishing)

### 12. `amqp_basic_publish`
```c
int amqp_basic_publish(amqp_connection_state_t state, amqp_channel_t channel, 
    amqp_bytes_t exchange, amqp_bytes_t routing_key, amqp_boolean_t mandatory, 
    amqp_boolean_t immediate, const amqp_basic_properties_t *properties, 
    amqp_bytes_t body);
```
*   **行为**：将消息发布到指定的交换机。
*   **参数**：
    *   `exchange`：目标交换机名（空字符串表示默认交换机）。
    *   `routing_key`：路由键（队列名若发往默认交换机）。
    *   `mandatory`：如果消息无法路由是否返回给发送者（需配合 `Return` 监听）。
    *   `immediate`：*（AMQP 0-9-1 中已弃用，通常传 `0`）*。
    *   `properties`：消息属性（包含 `content_type`, `delivery_mode`(持久化=2), `priority`, `reply_to` 等）。若无需特殊属性，传 `NULL`。
    *   `body`：消息体（字节数组）。
*   **返回值**：成功返回 `AMQP_STATUS_OK`，失败返回负错误码（如 `AMQP_STATUS_CONNECTION_CLOSED`）。
*   **同步/异步**：**I/O 同步阻塞，但逻辑异步（Fire-and-Forget）**。
    *   *阻塞行为*：此函数会尝试将帧写入内核 Socket 缓冲区。如果 Socket 缓冲区满，它会阻塞等待发送完成。
    *   *逻辑行为*：它**不等待** Broker 的确认（`Basic.Ack`）。如果 Broker 拒绝或无法路由（且 mandatory=1），Broker 会异步返回 `Basic.Return`，但此函数不会等待它。若要发布者确认（Publisher Confirms），需手动处理后续的 `wait_frame`。
*   **注意**：这是最常用的发布 API，性能高，因为不阻塞等待 Broker 逻辑响应。

---

## 六、消息消费 (Consuming)

### 13. `amqp_basic_consume`
```c
amqp_rpc_reply_t amqp_basic_consume(amqp_connection_state_t state, amqp_channel_t channel, 
    amqp_bytes_t queue, amqp_bytes_t consumer_tag, amqp_boolean_t no_local, 
    amqp_boolean_t no_ack, amqp_boolean_t exclusive, const amqp_table_t *arguments);
```
*   **行为**：注册消费者（订阅模式）。Broker 将开始向此连接推送消息。
*   **参数**：
    *   `queue`：要消费的队列名。
    *   `consumer_tag`：消费者标识（可传入 `amqp_empty_bytes` 让 Broker 自动生成）。**需保存此 Tag 用于取消消费**。
    *   `no_local`：是否接收自己发布的消息（通常设为 `0`）。
    *   `no_ack`：是否自动确认。设为 `1` 则 Broker 发送后立即删除，设为 `0` 需手动调用 `amqp_basic_ack`。
    *   `exclusive`：独占消费。
    *   `arguments`：扩展参数（如 `x-priority`），传 `amqp_empty_table`。
*   **返回值**：RPC 回复。成功时，`reply.reply.consumer_tag` 包含 Consumer Tag。
*   **同步/异步**：**同步（阻塞）**。等待 Broker 的 `Consume-Ok`。

---

### 14. `amqp_consume_message` (高级辅助函数)
```c
amqp_rpc_reply_t amqp_consume_message(amqp_connection_state_t state, amqp_message_t *message, 
    struct timeval *timeout);
```
*   **行为**：从已订阅的通道中阻塞获取下一帧并组装成完整的 `amqp_message_t` 结构。此函数处理了底层的帧解析（Header + Body）。
*   **参数**：
    *   `message`：输出参数，填充消息体、属性、Delivery Tag 等。
    *   `timeout`：超时时间。传入 `NULL` 表示无限阻塞。传入 `&(struct timeval){0,0}` 表示非阻塞轮询。
*   **返回值**：
    *   `reply_type == AMQP_RESPONSE_NORMAL`：成功读取一条消息。
    *   `reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION` 且 `reply.library_error == AMQP_STATUS_TIMEOUT`：超时。
    *   `reply_type == AMQP_RESPONSE_NONE`：连接关闭。
*   **同步/异步**：**同步（阻塞/带超时）**。根据 `timeout` 设置决定。
*   **注意**：读取后**必须**检查 `message->delivery_tag`，以便后续确认。

---

### 15. `amqp_simple_wait_frame` (底层帧等待)
```c
amqp_rpc_reply_t amqp_simple_wait_frame(amqp_connection_state_t state, amqp_frame_t *decoded_frame);
```
*   **行为**：等待并解析一个完整的 AMQP 帧（如心跳、`Basic.Deliver` 帧头）。
*   **参数**：`decoded_frame` 输出帧结构。
*   **返回值**：同 `amqp_consume_message`。
*   **同步/异步**：**同步（阻塞）**。
*   **变体（非阻塞）**：`amqp_simple_wait_frame_noblock`。如果无帧可读，立即返回 `AMQP_STATUS_TIMEOUT`（即使未设置超时）。此函数可用于事件循环（Epoll/Select）。

---

### 16. `amqp_basic_cancel`
```c
amqp_rpc_reply_t amqp_basic_cancel(amqp_connection_state_t state, amqp_channel_t channel, 
    amqp_bytes_t consumer_tag);
```
*   **行为**：取消订阅，Broker 停止推送消息。
*   **参数**：`consumer_tag`（从 `amqp_basic_consume` 返回的标签）。
*   **返回值**：RPC 回复。
*   **同步/异步**：**同步（阻塞）**。

---

## 七、拉取消息 (Pull - Get)

### 17. `amqp_basic_get`
```c
amqp_rpc_reply_t amqp_basic_get(amqp_connection_state_t state, amqp_channel_t channel, 
    amqp_bytes_t queue, amqp_boolean_t no_ack);
```
*   **行为**：主动从队列拉取一条消息（非订阅模式）。
*   **参数**：`no_ack` 是否自动确认。
*   **返回值**：此函数返回的 RPC 包含特殊信息。
    *   检查 `reply.reply_type`。
    *   若 `reply.id == AMQP_BASIC_GET_EMPTY_METHOD`：队列为空。
    *   若 `reply.id == AMQP_BASIC_GET_OK_METHOD`：消息获取成功。但消息体未包含在此回复中，需调用 `amqp_read_message` 读取后续的帧。
*   **同步/异步**：**同步（阻塞）**。

---

## 八、消息确认 (Acknowledgments)

### 18. `amqp_basic_ack`
```c
int amqp_basic_ack(amqp_connection_state_t state, amqp_channel_t channel, 
    uint64_t delivery_tag, amqp_boolean_t multiple);
```
*   **行为**：确认一条或多条消息已成功处理（仅在 `no_ack=0` 时需要）。
*   **参数**：
    *   `delivery_tag`：从 `amqp_message_t` 或 `amqp_frame_t.delivery.delivery_tag` 获取。
    *   `multiple`：`1` 表示确认该 delivery_tag 及之前所有未确认的消息（批量 ACK）；`0` 仅确认当前一条。
*   **返回值**：成功返回 `AMQP_STATUS_OK`。
*   **同步/异步**：**I/O 同步阻塞（发送帧），逻辑异步**。它不等待 Broker 的 ACK 回复（Broker 对此操作无回复帧）。

---

### 19. `amqp_basic_nack`
```c
int amqp_basic_nack(amqp_connection_state_t state, amqp_channel_t channel, 
    uint64_t delivery_tag, amqp_boolean_t multiple, amqp_boolean_t requeue);
```
*   **行为**：否定确认（拒绝消息），可选是否重新入队。
*   **参数**：
    *   `requeue`：`1` 放回队列重新投递，`0` 直接丢弃或进入死信队列。
*   **同步/异步**：同 ACK。

---

### 20. `amqp_basic_reject`
```c
int amqp_basic_reject(amqp_connection_state_t state, amqp_channel_t channel, 
    uint64_t delivery_tag, amqp_boolean_t requeue);
```
*   **行为**：拒绝单条消息（不支持批量）。
*   **注意**：`nack` 是 RabbitMQ 扩展，`reject` 是 AMQP 标准。推荐优先使用 `nack` 支持批量。

---

## 九、事务 (Transactions) - 可选

### 21. `amqp_tx_select`
```c
amqp_rpc_reply_t amqp_tx_select(amqp_connection_state_t state, amqp_channel_t channel);
```
*   **行为**：开启事务模式（性能较低，不推荐生产环境，建议使用 Publisher Confirms）。
*   **同步/异步**：阻塞 RPC。

### 22. `amqp_tx_commit` / `amqp_tx_rollback`
```c
amqp_rpc_reply_t amqp_tx_commit(...);
amqp_rpc_reply_t amqp_tx_rollback(...);
```
*   提交或回滚事务。阻塞 RPC。

---

## 十、错误处理与辅助函数

### 23. `amqp_get_rpc_reply`
```c
amqp_rpc_reply_t amqp_get_rpc_reply(amqp_connection_state_t state);
```
*   **行为**：获取最近一次 RPC 调用的回复（通常用于异步错误检查）。
*   **同步/异步**：非阻塞（纯获取）。

### 24. 错误码检查宏
常用错误码定义在 `<amqp_status.h>`：
*   `AMQP_STATUS_OK` (0)
*   `AMQP_STATUS_CONNECTION_CLOSED`
*   `AMQP_STATUS_TIMEOUT`
*   `AMQP_STATUS_SOCKET_ERROR`
*   `AMQP_STATUS_INVALID_PARAMETER`

---

## 注意事项 (Best Practices)
1.  **心跳机制**：建议设置 `heartbeat` 为 30~60 秒，防止防火墙踢掉空闲连接。
2.  **错误处理**：**每一次 RPC 调用都必须检查 `amqp_rpc_reply_t`**，否则 Broker 返回的异常帧会堆积，导致后续操作卡死。
3.  **内存管理**：`amqp_bytes_t` 可能是动态分配的（如从 `amqp_consume_message` 读取的消息体）。读取后需调用 `amqp_destroy_message(&msg)` 或 `amqp_free(message.body.bytes)` 防止内存泄漏。
4.  **多线程**：RabbitMQ-C **不是线程安全**的。通常一个连接对象专属于一个线程，或由外部互斥锁保护所有调用。
5.  **非阻塞模式**：如需集成到 Reactor/EventLoop，使用 `amqp_socket_get_sockfd()` 获取 fd，利用 `select/epoll` 监听可读事件，然后调用 `amqp_simple_wait_frame_noblock` 或 `amqp_consume_message` 带超时（0秒）来轮询。

此文档覆盖了 90% 生产场景所需的 API。如需更底层（如动态参数表操作），可参考官方源码。