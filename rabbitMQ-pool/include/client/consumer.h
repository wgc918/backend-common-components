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

/// 消息处理回调：收到消息时调用，envelope 由调用方管理生命周期
using MessageHandler = std::function<void(const amqp_envelope_t& envelope)>;

/// 错误处理回调
using ConsumerErrorHandler = std::function<void(const std::string& error)>;

/// Consumer 负责从 RabbitMQ 队列消费消息。
/// 持有专用 Channel（不归还池中），在独立线程中运行消费循环。
/// 通过回调函数通知用户收到消息或发生错误。
class Consumer
{
public:
    explicit Consumer(ChannelPool& pool);
    ~Consumer();

    Consumer(const Consumer& other)            = delete;
    Consumer& operator=(const Consumer& other) = delete;
    Consumer(Consumer&& other)                 = delete;
    Consumer& operator=(Consumer&& other)      = delete;

    // ---- 链式配置 ----

    /// 设置要消费的队列名称
    Consumer& set_queue(const std::string& queue);

    /// 设置消费者标签（可选，为空则由服务器自动生成）
    Consumer& set_consumer_tag(const std::string& tag);

    /// 设置是否自动确认（默认 true）
    Consumer& set_no_ack(bool no_ack);

    /// 设置预取计数（0 表示不限制）
    Consumer& set_prefetch_count(int count);

    /// 设置消费超时（默认 永久）
    Consumer& set_timeout(int timeout_ms);

    // ---- 回调设置 ----

    /// 设置收到消息时的回调
    Consumer& on_message(MessageHandler handler);

    /// 设置发生错误时的回调
    Consumer& on_error(ConsumerErrorHandler handler);

    // ---- 生命周期 ----

    /// 启动消费线程
    void start();

    /// 请求停止消费
    void stop();

    /// 等待消费线程结束
    void join();

    /// 是否正在运行
    bool is_running() const
    {
        return m_running.load();
    }

private:
    /// 内部消费循环，运行在独立线程中
    void consume_loop();

    ChannelPool& m_channel_pool;
    std::string  m_queue;
    std::string  m_consumer_tag;
    bool         m_no_ack;
    int          m_prefetch_count;
    int          m_timeout_ms;

    MessageHandler       m_message_handler;
    ConsumerErrorHandler m_error_handler;

    std::thread       m_thread;
    std::atomic<bool> m_running;
};

}  // namespace rmq