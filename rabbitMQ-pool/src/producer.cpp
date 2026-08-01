#include "../include/client/producer.h"

#include <iostream>

namespace rmq
{

Producer::Producer(ChannelPool& pool) : m_channel_pool(pool)
{
}

bool Producer::send(const Message& message)
{
    auto guard = m_channel_pool.acquire();
    if (!guard)
    {
        std::cerr << "[Error] Producer::send: Failed to acquire channel" << std::endl;
        return false;
    }

    amqp_basic_properties_t props = message.properties();

    return guard->basic_publish(message.exchange(), message.routing_key(),
                                &props, message.body_bytes());
}

bool Producer::send(const std::string& exchange, const std::string& routing_key,
                     const std::string& body, const amqp_basic_properties_t* props)
{
    auto guard = m_channel_pool.acquire();
    if (!guard)
    {
        std::cerr << "[Error] Producer::send: Failed to acquire channel" << std::endl;
        return false;
    }

    amqp_bytes_t body_bytes;
    body_bytes.len   = body.size();
    body_bytes.bytes = const_cast<char*>(body.data());

    return guard->basic_publish(exchange, routing_key, props, body_bytes);
}

}  // namespace rmq