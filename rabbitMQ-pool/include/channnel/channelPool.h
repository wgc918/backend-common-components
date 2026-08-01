//-----------------------------------------------------------------------------
// 版权所有 (C) 2026 rabbitMQ-pool 项目
//
// 文件: channelPool.h
// 作者: wgc
// 创建日期: 2026年7月
// 最后修改: 2026年8月
//
// 描述:
//     管理 Channel 对象池，提供线程安全的借出/归还机制。
//
// 功能特性:
//     - ChannelPool 类：预创建指定数量的 Channel，统一管理
//     - acquire()：阻塞获取可用 Channel，返回 ChannelGuard
//     - try_acquire()：带超时的获取，超时返回空 guard
//     - ChannelGuard 析构时自动归还 Channel 到池中
//
// 实现说明:
//     使用 std::queue 存储空闲 Channel，通过 std::mutex 和
//     std::condition_variable 实现线程安全的等待-通知机制。
//
// 注意事项:
//     - 禁止拷贝和移动
//     - ChannelPool 析构时会释放所有 Channel
//
// 许可证:
//     MIT License
//
//     版权所有 (c) 2026 wgc
//
//     特此免费授予获得本软件副本和相关文档文件（以下简称"软件"）的任何人
//     以处理软件的权利，包括但不限于使用、复制、修改、合并、出版、分发、
//     再许可和/或出售软件副本，以及允许软件适用者这样做，须在下列条件下：
//
//     上述版权声明和本许可声明应包含在软件的所有副本或实质性部分中。
//
//     软件按"原样"提供，不提供任何形式的明示或暗示的保证，
//     包括但不限于对适销性、特定用途适用性和非侵权性的保证。
//     在任何情况下，作者或版权持有人均不对任何索赔、损害或其他责任负责，
//     无论是在合同诉讼、侵权诉讼或其他诉讼中，
//     由于软件或软件的使用或其他交易产生的。
//-----------------------------------------------------------------------------

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

/// @brief Channel 对象池
/// @details
///     预创建指定数量的 Channel，提供线程安全的借出/归还机制。
///     通过 acquire() 阻塞获取可用 Channel，ChannelGuard 析构时自动归还。
class ChannelPool
{
public:
    // ============================================
    // 构造函数与析构函数
    // ============================================

    /// @brief 构造 Channel 对象池
    /// @param connection 共享的 RabbitMQ 连接
    /// @param channel_max 池中 Channel 最大数量
    ChannelPool(Connection& connection, int channel_max);
    ~ChannelPool();

    // 禁止拷贝和移动
    ChannelPool(const ChannelPool& other)            = delete;
    ChannelPool& operator=(const ChannelPool& other) = delete;
    ChannelPool(ChannelPool&& other)                 = delete;
    ChannelPool& operator=(ChannelPool&& other)      = delete;

    // ============================================
    // Channel 获取
    // ============================================

    /// @brief 阻塞获取可用 Channel，直到有 Channel 归还
    /// @return 持有 Channel 的 ChannelGuard
    ChannelGuard acquire();

    /// @brief 带超时的获取
    /// @param timeout 等待超时时间
    /// @return 成功时返回持有 Channel 的 ChannelGuard，超时返回空
    ///         guard（operator bool 为 false）
    ChannelGuard try_acquire(std::chrono::milliseconds timeout);

    // ============================================
    // 容量查询
    // ============================================

    /// @brief 当前可用的 Channel 数量
    size_t available() const;

    /// @brief 池中 Channel 总数
    size_t size() const;

    /// @brief 获取底层连接引用
    Connection& connection();

private:
    /// @brief 归还 Channel 到池中，由 ChannelGuard 析构调用
    /// @param channel 要归还的 Channel
    void release(std::unique_ptr<Channel> channel);
    friend class ChannelGuard;

private:
    Connection&                          m_connection;   ///< 共享连接
    int                                  m_channel_max;  ///< 最大 Channel 数
    std::queue<std::unique_ptr<Channel>> m_channels;     ///< 空闲 Channel 队列
    mutable std::mutex                   m_mutex;        ///< 线程安全锁
    std::condition_variable              m_cv;           ///< 条件变量
};

}  // namespace rmq