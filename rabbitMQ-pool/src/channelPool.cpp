#include <iostream>
#include <mutex>

#include "../include/channnel/channelPool.h"

namespace rmq
{

ChannelPool::ChannelPool(Connection& connection, int channel_max)
    : m_connection(connection), m_channel_max(channel_max)
{
    for (int i = 1; i < m_channel_max + 1; i++)
    {
        auto channel = std::make_unique<Channel>(m_connection, i);
        if (channel->open())
        {
            m_channels.push(std::move(channel));
        }
        else
        {
            std::cerr << "[Warning] ChannelPool: Failed to open channel " << i << std::endl;
        }
    }
}

ChannelPool::~ChannelPool()
{
    // 逐一关闭所有 channel
    std::lock_guard<std::mutex> lock(m_mutex);
    while (!m_channels.empty())
    {
        auto channel = std::move(m_channels.front());
        m_channels.pop();
        channel->close();
    }
}

ChannelGuard ChannelPool::acquire()
{
    std::unique_lock<std::mutex> locker(m_mutex);
    while (m_channels.empty())
    {
        m_cv.wait(locker);
    }
    auto channel = std::move(m_channels.front());
    m_channels.pop();
    return ChannelGuard(this, std::move(channel));
}

ChannelGuard ChannelPool::try_acquire(std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> locker(m_mutex);
    if (m_channels.empty())
    {
        m_cv.wait_for(locker, timeout);
        if (m_channels.empty())
        {
            return ChannelGuard(nullptr, nullptr);
        }
    }
    auto channel = std::move(m_channels.front());
    m_channels.pop();
    return ChannelGuard(this, std::move(channel));
}

size_t ChannelPool::available() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_channels.size();
}

size_t ChannelPool::size() const
{
    return m_channel_max;
}


Connection& ChannelPool::connection()
{
    return m_connection;
}

void ChannelPool::release(std::unique_ptr<Channel> channel)
{
    if (!channel)
        return;

    std::lock_guard<std::mutex> locker(m_mutex);
    m_channels.push(std::move(channel));
    m_cv.notify_one();
}

}  // namespace rmq