#include "../include/broker/queue.h"

namespace rmq
{

Queue::Queue(QueueConfig cfg) : m_cfg(std::move(cfg))
{
}

bool Queue::declare(Channel& channel) const
{
    return channel.queue_declare(m_cfg.name, false, m_cfg.durable,
                                 m_cfg.exclusive, m_cfg.auto_delete, m_cfg.arguments);
}

bool Queue::remove(Channel& channel, bool if_unused, bool if_empty) const
{
    return channel.queue_delete(m_cfg.name, if_unused, if_empty);
}

bool Queue::bind(Channel& channel, const std::string& exchange,
                 const std::string& routing_key) const
{
    return channel.queue_bind(m_cfg.name, exchange, routing_key);
}

bool Queue::unbind(Channel& channel, const std::string& exchange,
                   const std::string& routing_key) const
{
    return channel.queue_unbind(m_cfg.name, exchange, routing_key);
}

bool Queue::purge(Channel& channel) const
{
    return channel.queue_purge(m_cfg.name);
}

}  // namespace rmq