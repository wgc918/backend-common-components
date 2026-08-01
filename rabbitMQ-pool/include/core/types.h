#pragma once

#include <string>

namespace rmq
{

/// AMQP 交换机类型
enum class ExchangeType
{
    Direct,  // 路由键精确匹配
    Fanout,  // 广播到所有绑定的队列
    Topic,   // 路由键模式匹配（支持 * 和 #）
    Headers  // 基于消息头属性匹配
};

/// 将 ExchangeType 转换为 rabbitmq-c 所需的字符串
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