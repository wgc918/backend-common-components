//-----------------------------------------------------------------------------
// 版权所有 (C) 2026 rabbitMQ-pool 项目
//
// 文件: broker.h
// 作者: wgc
// 创建日期: 2026年8月
// 最后修改: 2026年8月
//
// 描述:
//     拓扑编排器，管理所有 Exchange 和 Queue 的声明与绑定。
//     配置与执行分离，支持批量声明与拆除。
//
// 功能特性:
//     - Broker 类：通过 add_exchange/add_queue 构建拓扑描述
//     - setup()：一次性执行所有 Exchange 和 Queue 的声明与绑定
//     - teardown()：一次性拆除所有拓扑（解绑、删除队列、删除交换机）
//     - 支持链式调用，便捷构建拓扑
//
// 实现说明:
//     Broker 不直接操作 AMQP，而是通过持有的 Exchange 和 Queue 对象，
//     在调用 setup/teardown 时逐个执行声明/删除操作。
//
// 使用场景:
//     - 应用启动时通过 Broker 一次性声明所有交换机、队列和绑定关系
//     - 应用关闭时通过 Broker 清理拓扑
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

#include <utility>
#include <vector>

#include "../channnel/channel.h"
#include "exchange.h"
#include "queue.h"

namespace rmq
{

/// @brief 拓扑编排器
/// @details
///     管理所有 Exchange 和 Queue 的声明与绑定。
///     配置与执行分离：先通过 add_exchange/add_queue 构建拓扑描述，
///     再通过 setup(Channel&) 一次性执行所有声明。
class Broker
{
public:
    // ============================================
    // 构造函数与析构函数
    // ============================================

    Broker() = default;

    // ============================================
    // 拓扑构建
    // ============================================

    /// @brief 添加交换机
    /// @param cfg 交换机配置
    /// @return Broker 引用，支持链式调用
    Broker& add_exchange(ExchangeConfig cfg);

    /// @brief 添加队列及其绑定关系
    /// @param cfg 队列配置
    /// @param bindings 绑定关系列表（可选）
    /// @return Broker 引用，支持链式调用
    Broker& add_queue(QueueConfig cfg, std::vector<Binding> bindings = {});

    // ============================================
    // 拓扑执行
    // ============================================

    /// @brief 通过指定的 Channel 声明所有交换机和队列，建立绑定关系
    /// @param channel 要使用的 Channel
    /// @return true 表示全部成功，false 表示中途有失败
    bool setup(Channel& channel);

    /// @brief 通过指定的 Channel 拆除拓扑：解绑、删除队列、删除交换机
    /// @param channel 要使用的 Channel
    /// @return true 表示全部成功，false 表示中途有失败
    bool teardown(Channel& channel);

    // ============================================
    // 查询
    // ============================================

    /// @brief 获取所有交换机
    const std::vector<Exchange>& exchanges() const;

    /// @brief 获取所有队列及其绑定
    const std::vector<std::pair<Queue, std::vector<Binding>>>& queues() const;

private:
    std::vector<Exchange>                               m_exchanges;  ///< 交换机列表
    std::vector<std::pair<Queue, std::vector<Binding>>> m_queues;     ///< 队列与绑定列表
};

}  // namespace rmq