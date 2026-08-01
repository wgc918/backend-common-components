#include "../include/channnel/channelGuard.h"

#include "../include/channnel/channelPool.h"

namespace rmq
{

ChannelGuard::ChannelGuard(ChannelPool* pool, std::unique_ptr<Channel> channel)
    : m_pool(pool), m_channel(std::move(channel))
{
}

ChannelGuard::ChannelGuard(ChannelGuard&& other) noexcept
    : m_pool(other.m_pool), m_channel(std::move(other.m_channel)), m_released(other.m_released)
{
    other.m_pool     = nullptr;
    other.m_released = true;
}

ChannelGuard& ChannelGuard::operator=(ChannelGuard&& other) noexcept
{
    if (this == &other)
        return *this;

    release();  // 先归还当前持有的 channel

    m_pool           = other.m_pool;
    m_channel        = std::move(other.m_channel);
    m_released       = other.m_released;
    other.m_pool     = nullptr;
    other.m_released = true;

    return *this;
}

ChannelGuard::~ChannelGuard()
{
    release();
}

Channel* ChannelGuard::operator->() const
{
    return m_channel.get();
}

Channel& ChannelGuard::operator*() const
{
    return *m_channel;
}

ChannelGuard::operator bool() const
{
    return m_channel != nullptr;
}

Channel* ChannelGuard::get() const
{
    return m_channel.get();
}

void ChannelGuard::release()
{
    if (!m_released && m_channel && m_pool != nullptr)
    {
        m_pool->release(std::move(m_channel));
        m_released = true;
    }
}

}  // namespace rmq