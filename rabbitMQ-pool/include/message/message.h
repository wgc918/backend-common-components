#pragma once

#include <amqp.h>

#include <cstring>
#include <string>

namespace rmq
{

/// Message 封装 AMQP 消息的完整信息：消息体、路由信息、属性。
/// 支持链式调用设置属性。
class Message
{
public:
    Message() = default;
    Message(std::string body, std::string exchange = "", std::string routing_key = "");

    // ---- 链式设置属性 ----

    Message& set_body(std::string body);
    Message& set_exchange(std::string exchange);
    Message& set_routing_key(std::string routing_key);
    Message& set_content_type(std::string type);
    Message& set_content_encoding(std::string encoding);
    Message& set_delivery_mode(uint8_t mode);  // 1=非持久化, 2=持久化
    Message& set_correlation_id(std::string id);
    Message& set_reply_to(std::string queue);
    Message& set_message_id(std::string id);
    Message& set_priority(uint8_t priority);
    Message& set_expiration(std::string expiration);
    Message& set_headers(const amqp_table_t& headers);

    // ---- 获取属性 ----

    const std::string& body() const
    {
        return m_body;
    }

    const std::string& exchange() const
    {
        return m_exchange;
    }

    const std::string& routing_key() const
    {
        return m_routing_key;
    }

    const std::string& content_type() const
    {
        return m_content_type;
    }

    const std::string& content_encoding() const
    {
        return m_content_encoding;
    }

    uint8_t delivery_mode() const
    {
        return m_delivery_mode;
    }

    const std::string& correlation_id() const
    {
        return m_correlation_id;
    }

    const std::string& reply_to() const
    {
        return m_reply_to;
    }

    const std::string& message_id() const
    {
        return m_message_id;
    }

    uint8_t priority() const
    {
        return m_priority;
    }

    const std::string& expiration() const
    {
        return m_expiration;
    }

    /// 构建 amqp_basic_properties_t 用于 publish
    amqp_basic_properties_t properties() const;

    /// 将 body 转换为 amqp_bytes_t
    amqp_bytes_t body_bytes() const;

private:
    // 内部辅助：将 std::string 转为 amqp_bytes_t（生命周期由 Message 对象管理）
    static amqp_bytes_t to_amqp_bytes(const std::string& s);

    std::string  m_body;
    std::string  m_exchange;
    std::string  m_routing_key;
    std::string  m_content_type = "text/plain";
    std::string  m_content_encoding;
    uint8_t      m_delivery_mode = 2;  // 默认持久化
    std::string  m_correlation_id;
    std::string  m_reply_to;
    std::string  m_message_id;
    uint8_t      m_priority = 0;
    std::string  m_expiration;
    amqp_table_t m_headers = amqp_empty_table;
};

}  // namespace rmq