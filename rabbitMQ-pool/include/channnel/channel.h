#pragma once

#include <amqp.h>

#include <cstdint>
#include <memory>
#include <string>

#include "../connection/connection.h"

namespace rmq
{

using channel_id = int;

/// Channel 封装单个 AMQP channel 的所有操作。
/// 每个方法内部自动获取 Connection 的互斥锁，保证线程安全。
class Channel
{
public:
    Channel(Connection& conn, channel_id id);
    ~Channel();

    // 禁止拷贝，允许移动
    Channel(const Channel& other)            = delete;
    Channel& operator=(const Channel& other) = delete;
    Channel(Channel&& other) noexcept;
    Channel& operator=(Channel&& other) noexcept;

    // 生命周期
    bool open();
    bool close();

    bool is_open() const
    {
        return m_open;
    }

    channel_id id() const
    {
        return m_channel_id;
    }

    // ---- Exchange 操作 ----
    bool exchange_declare(const std::string& name, const std::string& type, bool passive = false,
                          bool durable = false, bool auto_delete = false, bool internal = false,
                          amqp_table_t arguments = amqp_empty_table);
    bool exchange_delete(const std::string& name, bool if_unused = false);
    bool exchange_bind(const std::string& destination, const std::string& source,
                       const std::string& routing_key, amqp_table_t arguments = amqp_empty_table);
    bool exchange_unbind(const std::string& destination, const std::string& source,
                         const std::string& routing_key, amqp_table_t arguments = amqp_empty_table);

    // ---- Queue 操作 ----
    bool queue_declare(const std::string& name, bool passive = false, bool durable = false,
                       bool exclusive = false, bool auto_delete = false,
                       amqp_table_t arguments = amqp_empty_table);
    bool queue_delete(const std::string& name, bool if_unused = false, bool if_empty = false);
    bool queue_bind(const std::string& queue, const std::string& exchange,
                    const std::string& routing_key, amqp_table_t arguments = amqp_empty_table);
    bool queue_unbind(const std::string& queue, const std::string& exchange,
                      const std::string& routing_key, amqp_table_t arguments = amqp_empty_table);
    bool queue_purge(const std::string& queue);

    // ---- 消息操作 ----
    bool basic_publish(const std::string& exchange, const std::string& routing_key,
                       const amqp_basic_properties_t* props, const amqp_bytes_t& body,
                       bool mandatory = false, bool immediate = false);
    bool basic_consume(const std::string& queue, const std::string& consumer_tag = "",
                       bool no_local = false, bool no_ack = true, bool exclusive = false,
                       amqp_table_t arguments = amqp_empty_table);
    bool basic_ack(uint64_t delivery_tag, bool multiple = false);
    bool basic_nack(uint64_t delivery_tag, bool multiple = false, bool requeue = true);
    bool basic_qos(uint32_t prefetch_size, uint16_t prefetch_count, bool global = false);
    bool basic_cancel(const std::string& consumer_tag);

    // 设置超时（控制所有操作）
    void set_rpc_timeout(int timeout_ms);

    // ---- 消费消息（阻塞，带超时） ----
    // timeout_ms = -1 表示无限等待
    // 返回 true 表示成功获取消息，envelope 由调用方负责 amqp_destroy_envelope
    bool consume_message(amqp_envelope_t& envelope, int timeout_ms = -1);

    /// 获取底层连接引用
    Connection& connection()
    {
        return *m_connection;
    }

private:
    /// 检查 RPC 回复的辅助函数，失败时输出错误日志
    bool check_rpc_reply(const char* context);

    Connection* m_connection;
    channel_id  m_channel_id;
    bool        m_open = false;
};

}  // namespace rmq