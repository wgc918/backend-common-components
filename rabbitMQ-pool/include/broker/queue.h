//-----------------------------------------------------------------------------
// 版权所有 (C) 2026 rabbitMQ-pool 项目
//
// 文件: queue.h
// 作者: wgc
// 创建日期: 2026年8月
// 最后修改: 2026年8月
//
// 描述:
//     定义 Queue 配置、绑定关系及操作的值类型，封装队列的声明、删除、
//     绑定与清空等操作。
//
// 功能特性:
//     - QueueConfig 结构体：集中管理队列配置参数
//     - Binding 结构体：描述队列与交换机的绑定关系
//     - Queue 类：值类型，通过 declare/remove/bind/unbind/purge 管理队列
//     - 支持持久化、独占、自动删除等选项
//
// 使用场景:
//     - 与 Broker 配合使用，作为拓扑描述的一部分
//     - 也可独立使用，直接通过 Channel 操作队列
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
#include <vector>

#include "../channnel/channel.h"

namespace rmq
{

/// @brief 队列配置
struct QueueConfig
{
    std::string  name;                            ///< 队列名称
    bool         durable     = false;             ///< 是否持久化
    bool         exclusive   = false;             ///< 是否独占
    bool         auto_delete = true;              ///< 是否自动删除
    amqp_table_t arguments   = amqp_empty_table;  ///< 扩展参数
};

/// @brief 绑定关系
struct Binding
{
    std::string exchange;     ///< 交换机名称
    std::string routing_key;  ///< 路由键
};

/// @brief 队列值类型
/// @details
///     保存队列配置，通过 declare(Channel&) 在指定 Channel 上声明队列。
///     支持绑定、解绑、清空、删除等操作。
class Queue
{
public:
    // ============================================
    // 构造函数与析构函数
    // ============================================

    /// @brief 使用配置构造 Queue
    explicit Queue(QueueConfig cfg);

    // ============================================
    // 队列操作
    // ============================================

    /// @brief 通过指定的 Channel 声明队列
    /// @param channel 要使用的 Channel
    /// @return true 表示成功，false 表示失败
    bool declare(Channel& channel) const;

    /// @brief 删除队列
    /// @param channel 要使用的 Channel
    /// @param if_unused 仅在没有消费者使用时才删除
    /// @param if_empty 仅在队列为空时才删除
    /// @return true 表示成功，false 表示失败
    bool remove(Channel& channel, bool if_unused = false, bool if_empty = false) const;

    /// @brief 绑定队列到交换机
    /// @param channel 要使用的 Channel
    /// @param exchange 交换机名称
    /// @param routing_key 路由键
    /// @return true 表示成功，false 表示失败
    bool bind(Channel& channel, const std::string& exchange, const std::string& routing_key) const;

    /// @brief 解除绑定
    /// @param channel 要使用的 Channel
    /// @param exchange 交换机名称
    /// @param routing_key 路由键
    /// @return true 表示成功，false 表示失败
    bool unbind(Channel& channel, const std::string& exchange,
                const std::string& routing_key) const;

    /// @brief 清空队列
    /// @param channel 要使用的 Channel
    /// @return true 表示成功，false 表示失败
    bool purge(Channel& channel) const;

    // ============================================
    // 属性访问
    // ============================================

    /// @brief 获取队列名称
    const std::string& name() const;

private:
    QueueConfig m_cfg;  ///< 队列配置
};

}  // namespace rmq