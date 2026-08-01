#pragma once

#include <amqp.h>

#include <string>

#include "../channnel/channel.h"
#include "../core/types.h"

namespace rmq
{

/// Exchange 配置
struct ExchangeConfig
{
    std::string  name;
    ExchangeType type        = ExchangeType::Direct;
    bool         durable     = false;
    bool         auto_delete = false;
    bool         internal    = false;
    amqp_table_t arguments   = amqp_empty_table;
};

/// Exchange 是值类型，保存交换机配置。
/// 通过 declare(Channel&) 在指定 Channel 上声明交换机。
class Exchange
{
public:
    explicit Exchange(ExchangeConfig cfg);

    /// 通过指定的 Channel 声明交换机
    bool declare(Channel& channel) const;

    /// 删除交换机
    bool remove(Channel& channel, bool if_unused = false) const;

    const std::string& name() const
    {
        return m_cfg.name;
    }

    ExchangeType type() const
    {
        return m_cfg.type;
    }

private:
    ExchangeConfig m_cfg;
};

}  // namespace rmq