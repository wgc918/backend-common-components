#pragma once

#include <amqp.h>

#include <string>

#include "../channnel/channelPool.h"
#include "../message/message.h"

namespace rmq
{

/// Producer 负责发送消息到 RabbitMQ。
/// 通过 ChannelPool 获取 Channel，发送完成后自动归还。
class Producer
{
public:
    explicit Producer(ChannelPool& pool);

    Producer(const Producer& other)            = delete;
    Producer& operator=(const Producer& other) = delete;
    Producer(Producer&& other)                 = delete;
    Producer& operator=(Producer&& other)      = delete;

    /// 通过 Message 对象发送消息
    bool send(const Message& message);

    /// 直接发送（不通过 Message 对象）
    bool send(const std::string& exchange, const std::string& routing_key, const std::string& body,
              const amqp_basic_properties_t* props = nullptr);

    /// 批量发送，返回成功发送的数量
    template <typename Iterator>
    int send_batch(Iterator begin, Iterator end);

private:
    ChannelPool& m_channel_pool;
};

// ---- 模板实现 ----

template <typename Iterator>
int Producer::send_batch(Iterator begin, Iterator end)
{
    int count = 0;
    for (auto it = begin; it != end; ++it)
    {
        if (send(*it))
            ++count;
    }
    return count;
}

}  // namespace rmq