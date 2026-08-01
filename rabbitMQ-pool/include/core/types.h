//-----------------------------------------------------------------------------
// 版权所有 (C) 2026 rabbitMQ-pool 项目
//
// 文件: types.h
// 作者: wgc
// 创建日期: 2026年8月
// 最后修改: 2026年8月
//
// 描述:
//     定义 RabbitMQ 连接池的核心类型，包括交换机类型枚举及转换工具函数。
//
// 功能特性:
//     - ExchangeType 枚举：支持 Direct、Fanout、Topic、Headers 四种类型
//     - exchange_type_to_string：将枚举值转换为 rabbitmq-c 库所需字符串
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

#include <string>

namespace rmq
{

/// @brief AMQP 交换机类型枚举
enum class ExchangeType
{
    Direct,   ///< 路由键精确匹配
    Fanout,   ///< 广播到所有绑定的队列
    Topic,    ///< 路由键模式匹配（支持 * 和 #）
    Headers   ///< 基于消息头属性匹配
};

/// @brief 将 ExchangeType 转换为 rabbitmq-c 所需的字符串表示
/// @param type 交换机类型枚举值
/// @return 对应的字符串（"direct"、"fanout"、"topic"、"headers"）
inline const char* exchange_type_to_string(ExchangeType type)
{
    switch (type)
    {
        case ExchangeType::Direct:
            return "direct";
        case ExchangeType::Fanout:
            return "fanout";
        case ExchangeType::Topic:
            return "topic";
        case ExchangeType::Headers:
            return "headers";
    }
    return "direct";
}

}  // namespace rmq