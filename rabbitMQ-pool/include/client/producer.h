//-----------------------------------------------------------------------------
// 版权所有 (C) 2026 rabbitMQ-pool 项目
//
// 文件: producer.h
// 作者: wgc
// 创建日期: 2026年7月
// 最后修改: 2026年8月
//
// 描述:
//     负责发送消息到 RabbitMQ，通过 ChannelPool 获取 Channel，
//     发送完成后自动归还。
//
// 功能特性:
//     - Producer 类：支持单条、批量发送消息
//     - 通过 Message 对象发送，或直接指定 exchange、routing_key、body
//     - send_batch 模板方法支持迭代器范围的批量发送
//     - 内部通过 ChannelPool 获取 Channel，发送完成自动归还
//
// 使用场景:
//     - 生产者发送消息到指定交换机
//     - 批量发送多条消息
//
// 注意事项:
//     - 禁止拷贝和移动
//     - 每次 send 调用都会从 ChannelPool 借出并归还一个 Channel
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

#include <string>

#include "../channnel/channelPool.h"
#include "../message/message.h"

namespace rmq
{

/// @brief 消息生产者
/// @details
///     通过 ChannelPool 获取 Channel 发送消息，发送完成后自动归还。
///     支持单条、批量发送，支持通过 Message 对象或直接指定参数发送。
class Producer
{
public:
    // ============================================
    // 构造函数与析构函数
    // ============================================

    /// @brief 构造 Producer
    /// @param pool Channel 对象池引用
    explicit Producer(ChannelPool& pool);

    // 禁止拷贝和移动
    Producer(const Producer& other)            = delete;
    Producer& operator=(const Producer& other) = delete;
    Producer(Producer&& other)                 = delete;
    Producer& operator=(Producer&& other)      = delete;

    // ============================================
    // 发送消息
    // ============================================

    /// @brief 通过 Message 对象发送消息
    /// @param message 要发送的消息
    /// @return true 表示成功，false 表示失败
    bool send(const Message& message);

    /// @brief 直接发送（不通过 Message 对象）
    /// @param exchange 交换机名称
    /// @param routing_key 路由键
    /// @param body 消息体
    /// @param props 消息属性（可选）
    /// @return true 表示成功，false 表示失败
    bool send(const std::string& exchange, const std::string& routing_key, const std::string& body,
              const amqp_basic_properties_t* props = nullptr);

    /// @brief 批量发送消息
    /// @tparam Iterator 迭代器类型，解引用应得到 Message 对象
    /// @param begin 起始迭代器
    /// @param end 结束迭代器
    /// @return 成功发送的消息数量
    template <typename Iterator>
    int send_batch(Iterator begin, Iterator end);

private:
    ChannelPool& m_channel_pool;  ///< Channel 对象池
};

// ------------------------------ send_batch 模板实现 ------------------------------

template <typename Iterator>
int Producer::send_batch(Iterator begin, Iterator end)
{
    int count = 0;
    for (auto it = begin; it != end; ++it)
    {
        if (send(*it))
            count++;
    }
    return count;
}

}  // namespace rmq