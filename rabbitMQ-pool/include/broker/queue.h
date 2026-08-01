#pragma once

#include <amqp.h>

#include <string>
#include <vector>

#include "../channnel/channel.h"

namespace rmq
{

/// Queue 配置
struct QueueConfig
{
    std::string  name;
    bool         durable     = false;
    bool         exclusive   = false;
    bool         auto_delete = true;
    amqp_table_t arguments   = amqp_empty_table;
};

/// 绑定关系
struct Binding
{
    std::string exchange;
    std::string routing_key;
};

/// Queue 是值类型，保存队列配置。
class Queue
{
public:
    explicit Queue(QueueConfig cfg);

    /// 通过指定的 Channel 声明队列
    bool declare(Channel& channel) const;

    /// 删除队列
    bool remove(Channel& channel, bool if_unused = false, bool if_empty = false) const;

    /// 绑定队列到交换机
    bool bind(Channel& channel, const std::string& exchange, const std::string& routing_key) const;

    /// 解除绑定
    bool unbind(Channel& channel, const std::string& exchange,
                const std::string& routing_key) const;

    /// 清空队列
    bool purge(Channel& channel) const;

    const std::string& name() const
    {
        return m_cfg.name;
    }

private:
    QueueConfig m_cfg;
};

}  // namespace rmq