#pragma once

#include "../channnel/channelPool.h"
#include "./baseMessage.h"

namespace rmq
{
class Consumer
{
public:
    Consumer(const ChannelPool& pool);
    Consumer(const Consumer& other)            = delete;
    Consumer& operator=(const Consumer& other) = delete;
    Consumer(Consumer&& other)                 = delete;
    Consumer& operator=(Consumer&& other)      = delete;
    ~Consumer();

    BaseMessage recv();

private:
    const ChannelPool& m_channel_pool;
};
}  // namespace rmq