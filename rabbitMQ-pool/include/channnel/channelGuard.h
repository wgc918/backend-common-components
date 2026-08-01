#pragma once

#include <memory>

#include "channel.h"

namespace rmq
{

class ChannelPool;

/// ChannelGuard 是 RAII 守卫，析构时自动将 Channel 归还到 ChannelPool。
/// 支持移动语义，禁止拷贝。
class ChannelGuard
{
public:
    ChannelGuard(ChannelPool* pool, std::unique_ptr<Channel> channel);
    ~ChannelGuard();

    // 移动语义
    ChannelGuard(ChannelGuard&& other) noexcept;
    ChannelGuard& operator=(ChannelGuard&& other) noexcept;

    // 禁止拷贝
    ChannelGuard(const ChannelGuard&)            = delete;
    ChannelGuard& operator=(const ChannelGuard&) = delete;

    /// 通过 ChannelGuard 访问 Channel 的方法
    Channel* operator->() const
    {
        return m_channel.get();
    }

    Channel& operator*() const
    {
        return *m_channel;
    }

    /// 检查是否持有有效的 Channel
    explicit operator bool() const
    {
        return m_channel != nullptr;
    }

    /// 主动提前归还 Channel，归还后此 guard 不再持有
    void release();

    /// 获取裸指针（谨慎使用，不会转移所有权）
    Channel* get() const
    {
        return m_channel.get();
    }

private:
    ChannelPool*             m_pool = nullptr;
    std::unique_ptr<Channel> m_channel;
    bool                     m_released = false;
};

}  // namespace rmq