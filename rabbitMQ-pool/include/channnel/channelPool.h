#pragma once

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

#include "../connection/connection.h"
#include "channel.h"
#include "channelGuard.h"

namespace rmq
{

/// ChannelPool 管理 Channel 对象池，提供线程安全的借出/归还机制。
/// 预创建指定数量的 Channel，通过 acquire() 阻塞获取可用 Channel，
/// ChannelGuard 析构时自动归还。
class ChannelPool
{
public:
    /// @param connection 共享的 RabbitMQ 连接
    /// @param channel_max 池中 Channel 最大数量
    ChannelPool(Connection& connection, int channel_max);
    ~ChannelPool();

    ChannelPool(const ChannelPool& other)            = delete;
    ChannelPool& operator=(const ChannelPool& other) = delete;
    ChannelPool(ChannelPool&& other)                 = delete;
    ChannelPool& operator=(ChannelPool&& other)      = delete;

    /// 阻塞获取可用 Channel，直到有 Channel 归还
    ChannelGuard acquire();

    /// 带超时的获取，超时返回空的 ChannelGuard（operator bool 为 false）
    ChannelGuard try_acquire(std::chrono::milliseconds timeout);

    /// 当前可用的 Channel 数量
    size_t available() const;

    /// 池中 Channel 总数
    size_t size() const
    {
        return m_channel_max;
    }

    /// 获取底层连接引用
    Connection& connection()
    {
        return m_connection;
    }

private:
    /// 归还 Channel 到池中，由 ChannelGuard 析构调用
    void release(std::unique_ptr<Channel> channel);
    friend class ChannelGuard;

    Connection&                          m_connection;
    int                                  m_channel_max;
    std::queue<std::unique_ptr<Channel>> m_channels;
    mutable std::mutex                   m_mutex;
    std::condition_variable              m_cv;
};

}  // namespace rmq