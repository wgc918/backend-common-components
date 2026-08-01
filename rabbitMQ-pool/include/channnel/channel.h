//-----------------------------------------------------------------------------
// 版权所有 (C) 2026 rabbitMQ-pool 项目
//
// 文件: channel.h
// 作者: wgc
// 创建日期: 2026年8月
// 最后修改: 2026年8月
//
// 描述:
//     封装单个 AMQP channel 的所有操作，包括 Exchange/Queue 声明与绑定、
//     消息发布与消费等功能。
//
// 功能特性:
//     - Channel 类：封装 rabbitmq-c 的 channel 操作
//     - Exchange 操作：声明、删除、绑定、解绑
//     - Queue 操作：声明、删除、绑定、解绑、清空
//     - 消息操作：发布、消费、确认、拒绝、QoS 设置
//     - 每个方法内部自动获取 Connection 的互斥锁，保证线程安全
//
// 注意事项:
//     - 禁止拷贝，允许移动
//     - consume_message 为阻塞调用，支持超时
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

#include <cstdint>
#include <memory>
#include <string>

#include "../connection/connection.h"

namespace rmq
{

using channel_id = int;  ///< Channel ID 类型

/// @brief 封装单个 AMQP channel 的所有操作
/// @details
///     每个方法内部自动获取 Connection 的互斥锁，保证线程安全。
///     禁止拷贝，允许移动语义。
class Channel
{
public:
    // ============================================
    // 构造函数与析构函数
    // ============================================

    /// @brief 构造 Channel
    /// @param conn 所属的 Connection 引用
    /// @param id Channel ID
    Channel(Connection& conn, channel_id id);
    ~Channel();

    // 禁止拷贝，允许移动
    Channel(const Channel& other)            = delete;
    Channel& operator=(const Channel& other) = delete;
    Channel(Channel&& other) noexcept;
    Channel& operator=(Channel&& other) noexcept;

    // ============================================
    // 生命周期
    // ============================================

    /// @brief 打开 Channel
    /// @return true 表示成功，false 表示失败
    bool open();

    /// @brief 关闭 Channel
    /// @return true 表示成功，false 表示失败
    bool close();

    /// @brief 检查 Channel 是否处于打开状态
    bool is_open() const;

    /// @brief 获取 Channel ID
    channel_id id() const;

    // ============================================
    // Exchange 操作
    // ============================================

    /// @brief 声明交换机
    /// @param name 交换机名称
    /// @param type 交换机类型字符串（"direct"、"fanout"、"topic"、"headers"）
    /// @param passive 是否被动声明（仅检查，不创建）
    /// @param durable 是否持久化
    /// @param auto_delete 是否自动删除
    /// @param internal 是否为内部交换机
    /// @param arguments 扩展参数
    /// @return true 表示成功，false 表示失败
    bool exchange_declare(const std::string& name, const std::string& type, bool passive = false,
                          bool durable = false, bool auto_delete = false, bool internal = false,
                          amqp_table_t arguments = amqp_empty_table);

    /// @brief 删除交换机
    /// @param name 交换机名称
    /// @param if_unused 仅在没有队列使用时才删除
    /// @return true 表示成功，false 表示失败
    bool exchange_delete(const std::string& name, bool if_unused = false);

    /// @brief 绑定交换机到另一个交换机
    /// @param destination 目标交换机
    /// @param source 源交换机
    /// @param routing_key 路由键
    /// @param arguments 扩展参数
    /// @return true 表示成功，false 表示失败
    bool exchange_bind(const std::string& destination, const std::string& source,
                       const std::string& routing_key, amqp_table_t arguments = amqp_empty_table);

    /// @brief 解除交换机之间的绑定
    /// @param destination 目标交换机
    /// @param source 源交换机
    /// @param routing_key 路由键
    /// @param arguments 扩展参数
    /// @return true 表示成功，false 表示失败
    bool exchange_unbind(const std::string& destination, const std::string& source,
                         const std::string& routing_key, amqp_table_t arguments = amqp_empty_table);

    // ============================================
    // Queue 操作
    // ============================================

    /// @brief 声明队列
    /// @param name 队列名称
    /// @param passive 是否被动声明
    /// @param durable 是否持久化
    /// @param exclusive 是否独占
    /// @param auto_delete 是否自动删除
    /// @param arguments 扩展参数
    /// @return true 表示成功，false 表示失败
    bool queue_declare(const std::string& name, bool passive = false, bool durable = false,
                       bool exclusive = false, bool auto_delete = false,
                       amqp_table_t arguments = amqp_empty_table);

    /// @brief 删除队列
    /// @param name 队列名称
    /// @param if_unused 仅在没有消费者使用时才删除
    /// @param if_empty 仅在队列为空时才删除
    /// @return true 表示成功，false 表示失败
    bool queue_delete(const std::string& name, bool if_unused = false, bool if_empty = false);

    /// @brief 绑定队列到交换机
    /// @param queue 队列名称
    /// @param exchange 交换机名称
    /// @param routing_key 路由键
    /// @param arguments 扩展参数
    /// @return true 表示成功，false 表示失败
    bool queue_bind(const std::string& queue, const std::string& exchange,
                    const std::string& routing_key, amqp_table_t arguments = amqp_empty_table);

    /// @brief 解除队列与交换机的绑定
    /// @param queue 队列名称
    /// @param exchange 交换机名称
    /// @param routing_key 路由键
    /// @param arguments 扩展参数
    /// @return true 表示成功，false 表示失败
    bool queue_unbind(const std::string& queue, const std::string& exchange,
                      const std::string& routing_key, amqp_table_t arguments = amqp_empty_table);

    /// @brief 清空队列
    /// @param queue 队列名称
    /// @return true 表示成功，false 表示失败
    bool queue_purge(const std::string& queue);

    // ============================================
    // 消息操作
    // ============================================

    /// @brief 发布消息
    /// @param exchange 交换机名称
    /// @param routing_key 路由键
    /// @param props 消息属性
    /// @param body 消息体
    /// @param mandatory 是否强制路由
    /// @param immediate 是否立即投递
    /// @return true 表示成功，false 表示失败
    bool basic_publish(const std::string& exchange, const std::string& routing_key,
                       const amqp_basic_properties_t* props, const amqp_bytes_t& body,
                       bool mandatory = false, bool immediate = false);

    /// @brief 开始消费队列
    /// @param queue 队列名称
    /// @param consumer_tag 消费者标签（为空则由服务器自动生成）
    /// @param no_local 不接收本连接发布的消息
    /// @param no_ack 是否自动确认
    /// @param exclusive 是否独占消费
    /// @param arguments 扩展参数
    /// @return true 表示成功，false 表示失败
    bool basic_consume(const std::string& queue, const std::string& consumer_tag = "",
                       bool no_local = false, bool no_ack = true, bool exclusive = false,
                       amqp_table_t arguments = amqp_empty_table);

    /// @brief 确认消息
    /// @param delivery_tag 投递标签
    /// @param multiple 是否批量确认之前所有消息
    /// @return true 表示成功，false 表示失败
    bool basic_ack(uint64_t delivery_tag, bool multiple = false);

    /// @brief 拒绝消息
    /// @param delivery_tag 投递标签
    /// @param multiple 是否批量拒绝
    /// @param requeue 是否重新入队
    /// @return true 表示成功，false 表示失败
    bool basic_nack(uint64_t delivery_tag, bool multiple = false, bool requeue = true);

    /// @brief 设置 QoS 预取限制
    /// @param prefetch_size 预取大小
    /// @param prefetch_count 预取数量
    /// @param global 是否全局生效
    /// @return true 表示成功，false 表示失败
    bool basic_qos(uint32_t prefetch_size, uint16_t prefetch_count, bool global = false);

    /// @brief 取消消费
    /// @param consumer_tag 消费者标签
    /// @return true 表示成功，false 表示失败
    bool basic_cancel(const std::string& consumer_tag);

    /// @brief 设置 RPC 超时时间（控制所有操作）
    /// @param timeout_ms 超时毫秒数
    void set_rpc_timeout(int timeout_ms);

    // ============================================
    // 消费消息（阻塞）
    // ============================================

    /// @brief 消费消息（阻塞，带超时）
    /// @param envelope 输出参数，接收消息信封（调用方负责 amqp_destroy_envelope）
    /// @param timeout_ms 超时毫秒数（-1 表示无限等待）
    /// @return true 表示成功获取消息，false 表示超时或失败
    bool consume_message(amqp_envelope_t& envelope, int timeout_ms = -1);

    /// @brief 获取底层连接引用
    Connection& connection();

private:
    /// @brief 检查 RPC 回复的辅助函数，失败时输出错误日志
    /// @param context 操作上下文名称（用于日志）
    /// @return true 表示 RPC 调用成功，false 表示失败
    /// @note 失败时自动输出错误日志
    bool check_rpc_reply(const char* context);

private:
    Connection* m_connection;  ///< 所属连接
    channel_id  m_channel_id;  ///< Channel ID
    bool        m_open;        ///< 是否已打开
};

}  // namespace rmq