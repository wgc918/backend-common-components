#include "../include/client/consumer.h"

#include <iostream>

namespace rmq
{

Consumer::Consumer(ChannelPool& pool)
    : m_channel_pool(pool),
      m_no_ack(true),
      m_timeout_ms(-1),
      m_prefetch_count(0),
      m_consumer_tag("consumer_tag")
{
    m_running.store(false);
}

Consumer::~Consumer()
{
    stop();
    join();
}

Consumer& Consumer::set_queue(const std::string& queue)
{
    m_queue = queue;
    return *this;
}

Consumer& Consumer::set_consumer_tag(const std::string& tag)
{
    m_consumer_tag = tag;
    return *this;
}

Consumer& Consumer::set_no_ack(bool no_ack)
{
    m_no_ack = no_ack;
    return *this;
}

Consumer& Consumer::set_prefetch_count(int count)
{
    m_prefetch_count = count;
    return *this;
}

Consumer& Consumer::set_timeout(int timeout_ms)
{
    m_timeout_ms = timeout_ms;
    return *this;
}

Consumer& Consumer::on_message(MessageHandler handler)
{
    m_message_handler = std::move(handler);
    return *this;
}

Consumer& Consumer::on_error(ConsumerErrorHandler handler)
{
    m_error_handler = std::move(handler);
    return *this;
}

void Consumer::start()
{
    if (m_running.load())
    {
        std::cerr << "[Warning] Consumer::start: Already running" << std::endl;
        return;
    }

    if (m_queue.empty())
    {
        std::cerr << "[Error] Consumer::start: Queue name is not set" << std::endl;
        return;
    }

    m_running.store(true);
    m_thread = std::thread(&Consumer::consume_loop, this);
}

void Consumer::stop()
{
    m_running.store(false);
}

void Consumer::join()
{
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

void Consumer::consume_loop()
{
    // 获取专用 channel，不归还池中
    auto guard = m_channel_pool.acquire();
    if (!guard)
    {
        if (m_error_handler)
            m_error_handler("Failed to acquire channel for consumer");
        m_running.store(false);
        return;
    }

    // 设置 QoS
    if (m_prefetch_count > 0)
    {
        guard->basic_qos(0, m_prefetch_count, false);
    }

    // 开始消费
    if (!guard->basic_consume(m_queue, m_consumer_tag, false, m_no_ack, false))
    {
        if (m_error_handler)
            m_error_handler("Failed to start consuming queue: " + m_queue);
        m_running.store(false);
        return;
    }

    // 消费循环
    while (m_running.load())
    {
        amqp_envelope_t envelope;
        bool            ok = guard->consume_message(envelope, m_timeout_ms);

        if (ok)
        {
            if (m_message_handler)
                m_message_handler(envelope);

            if (!m_no_ack)
                guard->basic_ack(envelope.delivery_tag);

            amqp_destroy_envelope(&envelope);
        }
        else if (m_running.load())
        {
            // 超时或临时错误，继续循环
            if (m_error_handler)
                m_error_handler("consume_message timeout or temporary error");
        }
    }

    // 停止消费
    guard->basic_cancel(m_consumer_tag);
}

}  // namespace rmq