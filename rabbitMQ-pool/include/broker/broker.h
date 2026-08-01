#pragma once

#include <utility>
#include <vector>

#include "../channnel/channel.h"
#include "exchange.h"
#include "queue.h"

namespace rmq
{

/// Broker 是拓扑编排器，管理所有 Exchange 和 Queue 的声明与绑定。
/// 配置与执行分离：先通过 add_exchange/add_queue 构建拓扑描述，
/// 再通过 setup(Channel&) 一次性执行所有声明。
class Broker
{
public:
    Broker() = default;

    /// 添加交换机
    Broker& add_exchange(ExchangeConfig cfg);

    /// 添加队列及其绑定关系
    Broker& add_queue(QueueConfig cfg, std::vector<Binding> bindings = {});

    /// 通过指定的 Channel 声明所有交换机和队列，建立绑定关系
    /// @return true 表示全部成功，false 表示中途有失败
    bool setup(Channel& channel);

    /// 通过指定的 Channel 拆除拓扑：解绑、删除队列、删除交换机
    /// @return true 表示全部成功，false 表示中途有失败
    bool teardown(Channel& channel);

    /// 获取所有交换机
    const std::vector<Exchange>& exchanges() const
    {
        return m_exchanges;
    }

    /// 获取所有队列及其绑定
    const std::vector<std::pair<Queue, std::vector<Binding>>>& queues() const
    {
        return m_queues;
    }

private:
    std::vector<Exchange>                               m_exchanges;
    std::vector<std::pair<Queue, std::vector<Binding>>> m_queues;
};

}  // namespace rmq