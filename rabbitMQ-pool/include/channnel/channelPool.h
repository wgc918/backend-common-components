#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

#include "../connection/connection.h"

namespace rmq
{

using channel_id = int;

class ChannelPool
{
public:
    ChannelPool(const Connection& connection, int channel_max);
    ChannelPool(const ChannelPool& other)            = delete;
    ChannelPool& operator=(const ChannelPool& other) = delete;
    ChannelPool(ChannelPool&& other)                 = delete;
    ChannelPool& operator=(ChannelPool&& other)      = delete;
    ~ChannelPool()                                   = default;

private:
    channel_id get_channel();
    void return_channel(channel_id channel);

private:
    const Connection&       m_conn;
    std::queue<channel_id>  m_channels;
    int                     m_channel_max;
    std::mutex              m_mtx;
    std::condition_variable m_empty_cv;
};
}  // namespace rmq