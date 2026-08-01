//-----------------------------------------------------------------------------
// 版权所有 (C) 2026 rabbitMQ-pool 项目
//
// 文件: consumer.h
// 作者: wgc
// 创建日期: 2026年8月
// 最后修改: 2026年8月
//
// 描述:
//     负责从 RabbitMQ 队列消费消息，在独立线程中运行消费循环，
//     通过回调函数通知用户收到消息或发生错误。
//
// 功能特性:
//     - Consumer 类：持有专用 Channel，在独立线程中运行消费循环
//     - 链式配置：设置队列、消费者标签、自动确认、预取计数、超时等
//     - 回调机制：MessageHandler 处理消息，ConsumerErrorHandler 处理错误
//     - 线程安全：start/stop/join 控制消费线程生命周期
//
// 实现说明:
//     Consumer 从 ChannelPool 借出一个 Channel 后不归还，
//     在独立线程中循环调用 consume_message 阻塞等待消息。
//     收到消息后通过 MessageHandler 回调通知用户。
//
// 使用场景:
//     - 订阅队列消息，通过回调异步处理
//     - 需要独立线程持续消费的场景
//
// 注意事项:
//     - 禁止拷贝和移动
//     - 析构前应先调用 stop() + join() 确保线程安全退出
//     - Consumer 持有的 Channel 不会归还到池中
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

#include <amqp.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <thread>

#include "../channnel/channel.h"
#include "../channnel/channelPool.h"

namespace rmq
{

/// @brief 消息处理回调：收到消息时调用
/// @note envelope 由调用方管理生命周期
using MessageHandler = std::function<void(const amqp_envelope_t& envelope)>;

/// @brief 错误处理回调：发生错误时调用
using ConsumerErrorHandler = std::function<void(const std::string& error)>;

/// @brief 消息消费者
/// @details
///     持有专用 Channel（不归还池中），在独立线程中运行消费循环。
///     通过回调函数通知用户收到消息或发生错误。
///     支持链式配置各项参数。
class Consumer
{
public:
    // ============================================
    // 构造函数与析构函数
    // ============================================

    /// @brief 构造 Consumer
    /// @param pool Channel 对象池引用
    explicit Consumer(ChannelPool& pool);
    ~Consumer();

    // 禁止拷贝和移动
    Consumer(const Consumer& other)            = delete;
    Consumer& operator=(const Consumer& other) = delete;
    Consumer(Consumer&& other)                 = delete;
    Consumer& operator=(Consumer&& other)      = delete;

    // ============================================
    // 链式配置
    // ============================================

    /// @brief 设置要消费的队列名称
    /// @param queue 队列名称
    Consumer& set_queue(const std::string& queue);

    /// @brief 设置消费者标签
    /// @param tag 消费者标签（为空则由服务器自动生成）
    Consumer& set_consumer_tag(const std::string& tag);

    /// @brief 设置是否自动确认
    /// @param no_ack 是否自动确认（默认 true）
    Consumer& set_no_ack(bool no_ack);

    /// @brief 设置预取计数
    /// @param count 预取数量（0 表示不限制）
    Consumer& set_prefetch_count(int count);

    /// @brief 设置消费超时
    /// @param timeout_ms 超时毫秒数（默认永久）
    Consumer& set_timeout(int timeout_ms);

    // ============================================
    // 回调设置
    // ============================================

    /// @brief 设置收到消息时的回调
    Consumer& on_message(MessageHandler handler);

    /// @brief 设置发生错误时的回调
    Consumer& on_error(ConsumerErrorHandler handler);

    // ============================================
    // 生命周期
    // ============================================

    /// @brief 启动消费线程
    void start();

    /// @brief 请求停止消费
    void stop();

    /// @brief 等待消费线程结束
    void join();

    /// @brief 检查是否正在运行
    bool is_running() const;

private:
    /// @brief 内部消费循环，运行在独立线程中
    void consume_loop();

private:
    ChannelPool& m_channel_pool;    ///< Channel 对象池
    std::string  m_queue;           ///< 消费的队列名称
    std::string  m_consumer_tag;    ///< 消费者标签
    bool         m_no_ack;          ///< 是否自动确认
    int          m_prefetch_count;  ///< 预取计数
    int          m_timeout_ms;      ///< 消费超时毫秒

    MessageHandler       m_message_handler;  ///< 消息处理回调
    ConsumerErrorHandler m_error_handler;    ///< 错误处理回调

    std::thread       m_thread;   ///< 消费线程
    std::atomic<bool> m_running;  ///< 运行状态
};

}  // namespace rmq