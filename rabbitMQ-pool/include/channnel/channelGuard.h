//-----------------------------------------------------------------------------
// 版权所有 (C) 2026 rabbitMQ-pool 项目
//
// 文件: channelGuard.h
// 作者: wgc
// 创建日期: 2026年8月
// 最后修改: 2026年8月
//
// 描述:
//     RAII 守卫，析构时自动将 Channel 归还到 ChannelPool。
//
// 功能特性:
//     - ChannelGuard 类：通过 operator-> 和 operator* 访问 Channel
//     - 析构时自动归还 Channel，避免资源泄漏
//     - 支持移动语义，禁止拷贝
//     - 提供 release() 主动提前归还
//     - operator bool 判断是否持有有效 Channel
//
// 实现说明:
//     持有 ChannelPool 指针和 unique_ptr<Channel>，析构时调用
//     ChannelPool::release() 归还 Channel。
//
// 使用场景:
//     - 通过 ChannelPool::acquire() 获取 ChannelGuard
//     - 函数退出时自动归还 Channel，无需手动管理
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

#include <memory>

#include "channel.h"

namespace rmq
{

class ChannelPool;

/// @brief RAII 守卫，析构时自动归还 Channel
/// @details
///     持有 Channel 的独占所有权，析构时自动将 Channel 归还到
///     ChannelPool。支持移动语义，禁止拷贝。
class ChannelGuard
{
public:
    // ============================================
    // 构造函数与析构函数
    // ============================================

    /// @brief 构造 ChannelGuard
    /// @param pool 所属 ChannelPool 指针
    /// @param channel 要管理的 Channel
    ChannelGuard(ChannelPool* pool, std::unique_ptr<Channel> channel);
    ~ChannelGuard();

    // 移动语义
    ChannelGuard(ChannelGuard&& other) noexcept;
    ChannelGuard& operator=(ChannelGuard&& other) noexcept;

    // 禁止拷贝
    ChannelGuard(const ChannelGuard&)            = delete;
    ChannelGuard& operator=(const ChannelGuard&) = delete;

    // ============================================
    // Channel 访问
    // ============================================

    /// @brief 通过箭头操作符访问 Channel 的方法
    Channel* operator->() const;

    /// @brief 通过解引用操作符访问 Channel 的引用
    Channel& operator*() const;

    /// @brief 检查是否持有有效的 Channel
    explicit operator bool() const;

    // ============================================
    // 生命周期管理
    // ============================================

    /// @brief 主动提前归还 Channel，归还后此 guard 不再持有
    void release();

    /// @brief 获取 Channel 裸指针（谨慎使用，不会转移所有权）
    Channel* get() const;

private:
    ChannelPool*             m_pool = nullptr;    ///< 所属 ChannelPool
    std::unique_ptr<Channel> m_channel;           ///< 持有的 Channel
    bool                     m_released = false;  ///< 是否已归还
};

}  // namespace rmq