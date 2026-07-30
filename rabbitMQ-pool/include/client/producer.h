#pragma once

#include "../channnel/channelPool.h"
#include "./baseMessage.h"

namespace rmq
{
class Producer
{
public:
    Producer(const ChannelPool& pool);
    Producer(const Producer& other)            = delete;
    Producer& operator=(const Producer& other) = delete;
    Producer(Producer&& other)                 = delete;
    Producer& operator=(Producer&& other)      = delete;
    ~Producer();

    bool send(const BaseMessage& message);

private:
    const ChannelPool& m_channel_pool;
};
}  // namespace rmq