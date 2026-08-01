#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "../include/broker/broker.h"
#include "../include/broker/exchange.h"
#include "../include/broker/queue.h"
#include "../include/channnel/channelPool.h"
#include "../include/client/producer.h"
#include "../include/connection/connection.h"
#include "../include/message/message.h"

int main()
{
    using namespace rmq;

    const std::string exchange_name = "example_exchange";
    const std::string queue_name    = "example_queue";
    const std::string routing_key   = "example_key";

    // 1. 初始化连接
    ConnConfig cfg;
    Connection conn;
    if (!conn.init(cfg))
    {
        std::cerr << "Failed to init connection" << std::endl;
        return 1;
    }
    std::cout << "Connection established" << std::endl;

    // 2. 创建 ChannelPool
    ChannelPool pool(conn, 5);
    std::cout << "ChannelPool created with " << pool.size() << " channels" << std::endl;

    // 3. 配置 Broker 拓扑
    Broker broker;
    broker.add_exchange(ExchangeConfig{exchange_name, ExchangeType::Direct, false, false, false});
    broker.add_queue(
        QueueConfig{
            queue_name, false, false, true
    },
        {Binding{exchange_name, routing_key}});

    {
        auto guard = pool.acquire();
        if (!broker.setup(*guard))
        {
            std::cerr << "Failed to setup broker topology" << std::endl;
            return 1;
        }
    }
    std::cout << "Broker topology setup complete" << std::endl;

    // 4. 创建 Producer 并发送消息
    Producer producer(pool);

    for (int i = 1; i <= 10; ++i)
    {
        Message msg("Hello, RabbitMQ! Message number: " + std::to_string(i), exchange_name,
                    routing_key);
        msg.set_content_type("text/plain").set_delivery_mode(2);

        if (producer.send(msg))
        {
            std::cout << "Message " << i << " published" << std::endl;
        }
        else
        {
            std::cerr << "Failed to publish message " << i << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "Done. All messages sent." << std::endl;
    return 0;
}