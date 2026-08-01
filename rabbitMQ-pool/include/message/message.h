//-----------------------------------------------------------------------------
// 版权所有 (C) 2026 rabbitMQ-pool 项目
//
// 文件: message.h
// 作者: wgc
// 创建日期: 2026年8月
// 最后修改: 2026年8月
//
// 描述:
//     封装 AMQP 消息的完整信息，包括消息体、路由信息及属性，支持链式调用。
//
// 功能特性:
//     - Message 类：封装 body、exchange、routing_key 及 AMQP 属性
//     - 链式调用设置属性，提升代码可读性
//     - 提供 properties() 和 body_bytes() 方法，便于与 rabbitmq-c 库交互
//
// 实现说明:
//     内部使用 std::string 保存属性值，to_amqp_bytes 将其转换为
//     amqp_bytes_t 时生命周期由 Message 对象管理。
//
// 使用场景:
//     - 生产者通过 Message 对象构造消息后发送
//     - 消费者通过 Message 对象解析收到的消息
//
// 注意事项:
//     - m_headers 为 amqp_table_t 类型，赋值时需确保生命周期
//     - delivery_mode 默认为 2（持久化）
//
// 许可证:
//     MIT License
//
//     版权所有 (c) 2026 wgc
//
//     特此免费授予获得本软件副本和相关文档文件（以下简称"软件"）的任何人
//     以处理软件的权利，包括但不限于使用、复制、修改、合并、出版、分发、
//     再许可和/或出售软件副本，以及允许软件适用者这样做，须在下列条件下：
//
//     上述版权声明和本许可声明应包含在软件的所有副本或实质性部分中。
//
//     软件按"原样"提供，不提供任何形式的明示或暗示的保证，
//     包括但不限于对适销性、特定用途适用性和非侵权性的保证。
//     在任何情况下，作者或版权持有人均不对任何索赔、损害或其他责任负责，
//     无论是在合同诉讼、侵权诉讼或其他诉讼中，
//     由于软件或软件的使用或其他交易产生的。
//-----------------------------------------------------------------------------

#pragma once

#include <amqp.h>

#include <cstring>
#include <string>

namespace rmq
{

/// @brief 封装 AMQP 消息的完整信息
/// @details
///     包含消息体、路由信息及 AMQP 标准属性。
///     支持链式调用设置各项属性，提供便捷的构建体验。
class Message
{
public:
    // ============================================
    // 构造函数与析构函数
    // ============================================

    Message() = default;

    /// @brief 构造消息
    /// @param body 消息体
    /// @param exchange 交换机名称（可选）
    /// @param routing_key 路由键（可选）
    Message(std::string body, std::string exchange = "", std::string routing_key = "");

    // ============================================
    // 链式设置属性
    // ============================================

    /// @brief 设置消息体
    Message& set_body(std::string body);

    /// @brief 设置交换机名称
    Message& set_exchange(std::string exchange);

    /// @brief 设置路由键
    Message& set_routing_key(std::string routing_key);

    /// @brief 设置内容类型
    Message& set_content_type(std::string type);

    /// @brief 设置内容编码
    Message& set_content_encoding(std::string encoding);

    /// @brief 设置投递模式（1=非持久化，2=持久化）
    /// @param mode 投递模式
    Message& set_delivery_mode(uint8_t mode);

    /// @brief 设置关联 ID
    Message& set_correlation_id(std::string id);

    /// @brief 设置回复队列
    Message& set_reply_to(std::string queue);

    /// @brief 设置消息 ID
    Message& set_message_id(std::string id);

    /// @brief 设置优先级
    /// @param priority 优先级值
    Message& set_priority(uint8_t priority);

    /// @brief 设置过期时间
    Message& set_expiration(std::string expiration);

    /// @brief 设置消息头
    /// @param headers AMQP 消息头
    Message& set_headers(const amqp_table_t& headers);

    // ============================================
    // 获取属性
    // ============================================

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

    // ============================================
    // 转换方法
    // ============================================

    /// @brief 构建 amqp_basic_properties_t 用于 publish
    /// @return AMQP 基本属性结构体
    amqp_basic_properties_t properties() const;

    /// @brief 将 body 转换为 amqp_bytes_t
    /// @return body 的字节表示
    amqp_bytes_t body_bytes() const;

private:
    /// @brief 将 std::string 转为 amqp_bytes_t（生命周期由 Message 对象管理）
    static amqp_bytes_t to_amqp_bytes(const std::string& s);

private:
    std::string  m_body;
    std::string  m_exchange;
    std::string  m_routing_key;
    std::string  m_content_type = "text/plain";  ///< 默认纯文本
    std::string  m_content_encoding;
    uint8_t      m_delivery_mode = 2;  ///< 默认持久化
    std::string  m_correlation_id;
    std::string  m_reply_to;
    std::string  m_message_id;
    uint8_t      m_priority = 0;
    std::string  m_expiration;
    amqp_table_t m_headers = amqp_empty_table;
};

}  // namespace rmq