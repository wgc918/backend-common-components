#include "../include/broker/broker.h"

#include <iostream>

namespace rmq
{

Broker& Broker::add_exchange(ExchangeConfig cfg)
{
    m_exchanges.emplace_back(std::move(cfg));
    return *this;
}

Broker& Broker::add_queue(QueueConfig cfg, std::vector<Binding> bindings)
{
    m_queues.emplace_back(Queue(std::move(cfg)), std::move(bindings));
    return *this;
}

bool Broker::setup(Channel& channel)
{
    bool all_ok = true;

    // 1. 声明所有交换机
    for (const auto& exchange : m_exchanges)
    {
        if (!exchange.declare(channel))
        {
            std::cerr << "[Error] Broker::setup: Failed to declare exchange '" << exchange.name()
                      << "'" << std::endl;
            all_ok = false;
        }
    }

    // 2. 声明所有队列
    for (auto& [queue, bindings] : m_queues)
    {
        if (!queue.declare(channel))
        {
            std::cerr << "[Error] Broker::setup: Failed to declare queue '" << queue.name() << "'"
                      << std::endl;
            all_ok = false;
            continue;  // 跳过此队列的绑定
        }

        // 3. 建立绑定
        for (const auto& binding : bindings)
        {
            if (!queue.bind(channel, binding.exchange, binding.routing_key))
            {
                std::cerr << "[Error] Broker::setup: Failed to bind queue '" << queue.name()
                          << "' to exchange '" << binding.exchange << "' with routing_key '"
                          << binding.routing_key << "'" << std::endl;
                all_ok = false;
            }
        }
    }

    return all_ok;
}

bool Broker::teardown(Channel& channel)
{
    bool all_ok = true;

    // 1. 解绑所有队列
    for (auto& [queue, bindings] : m_queues)
    {
        for (const auto& binding : bindings)
        {
            queue.unbind(channel, binding.exchange, binding.routing_key);
        }
    }

    // 2. 删除所有队列
    for (auto& [queue, bindings] : m_queues)
    {
        if (!queue.remove(channel, false, false))
        {
            std::cerr << "[Error] Broker::teardown: Failed to remove queue '" << queue.name() << "'"
                      << std::endl;
            all_ok = false;
        }
    }

    // 3. 删除所有交换机
    for (const auto& exchange : m_exchanges)
    {
        if (!exchange.remove(channel, false))
        {
            std::cerr << "[Error] Broker::teardown: Failed to remove exchange '" << exchange.name()
                      << "'" << std::endl;
            all_ok = false;
        }
    }

    return all_ok;
}

const std::vector<Exchange>& Broker::exchanges() const
{
    return m_exchanges;
}

const std::vector<std::pair<Queue, std::vector<Binding>>>& Broker::queues() const
{
    return m_queues;
}

}  // namespace rmq