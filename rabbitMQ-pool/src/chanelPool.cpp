#include <amqp.h>

#include <iostream>
#include <mutex>

#include "../include/channnel/channelPool.h"

namespace rmq
{
ChannelPool::ChannelPool(const Connection& connection, int channel_max)
    : m_conn(connection), m_channel_max(channel_max)
{
    for (int i = 1; i < m_channel_max + 1; i++)
    {
        amqp_channel_open(m_conn.connection(), i);
        auto reply = amqp_get_rpc_reply(m_conn.connection());
        if (reply.reply_type != AMQP_RESPONSE_NORMAL)
        {
            std::cerr << "[Warning] Failed to open channel" << i << std::endl;
        }
    }
}

channel_id ChannelPool::get_channel()
{
    std::unique_lock<std::mutex> locker(m_mtx);
    while (m_channels.empty())
    {
        m_empty_cv.wait(locker);
    }
    channel_id channel = m_channels.front();
    m_channels.pop();
    return channel;
}

void ChannelPool::return_channel(channel_id channel)
{
    if (channel < 0)
        return;
    std::unique_lock<std::mutex> locker(m_mtx);
    m_channels.push(channel);
    m_empty_cv.notify_one();
}

}  // namespace rmq