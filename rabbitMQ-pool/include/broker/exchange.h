//-----------------------------------------------------------------------------
// 版权所有 (C) 2026 rabbitMQ-pool 项目
//
// 文件: exchange.h
// 作者: wgc
// 创建日期: 2026年8月
// 最后修改: 2026年8月
//
// 描述:
//     定义 Exchange 配置与操作的值类型，封装交换机的声明与删除。
//
// 功能特性:
//     - ExchangeConfig 结构体：集中管理交换机配置参数
//     - Exchange 类：值类型，通过 declare/remove 管理交换机
//     - 支持持久化、自动删除、内部交换机等选项
//
// 使用场景:
//     - 与 Broker 配合使用，作为拓扑描述的一部分
//     - 也可独立使用，直接通过 Channel 声明交换机
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

#include "../channnel/channel.h"
#include "../core/types.h"

namespace rmq
{

/// @brief 交换机配置
struct ExchangeConfig
{
    std::string  name;                                ///< 交换机名称
    ExchangeType type        = ExchangeType::Direct;  ///< 交换机类型
    bool         durable     = false;                 ///< 是否持久化
    bool         auto_delete = false;                 ///< 是否自动删除
    bool         internal    = false;                 ///< 是否为内部交换机
    amqp_table_t arguments   = amqp_empty_table;      ///< 扩展参数
};

/// @brief 交换机值类型
/// @details
///     保存交换机配置，通过 declare(Channel&) 在指定 Channel 上
///     声明交换机。支持 remove 删除交换机。
class Exchange
{
public:
    // ============================================
    // 构造函数与析构函数
    // ============================================

    /// @brief 使用配置构造 Exchange
    explicit Exchange(ExchangeConfig cfg);

    // ============================================
    // 交换机操作
    // ============================================

    /// @brief 通过指定的 Channel 声明交换机
    /// @param channel 要使用的 Channel
    /// @return true 表示成功，false 表示失败
    bool declare(Channel& channel) const;

    /// @brief 删除交换机
    /// @param channel 要使用的 Channel
    /// @param if_unused 仅在没有队列使用时才删除
    /// @return true 表示成功，false 表示失败
    bool remove(Channel& channel, bool if_unused = false) const;

    // ============================================
    // 属性访问
    // ============================================

    /// @brief 获取交换机名称
    const std::string& name() const;

    /// @brief 获取交换机类型
    ExchangeType type() const;

private:
    ExchangeConfig m_cfg;  ///< 交换机配置
};

}  // namespace rmq