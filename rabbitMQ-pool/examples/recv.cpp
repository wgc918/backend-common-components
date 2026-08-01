#include <atomic>
#include <csignal>
#include <iostream>
#include <string>

#include "../include/broker/broker.h"
#include "../include/broker/exchange.h"
#include "../include/broker/queue.h"
#include "../include/channnel/channelPool.h"
#include "../include/client/consumer.h"
#include "../include/connection/connection.h"

std::atomic<bool> g_running{true};

void signal_handler(int /*signum*/)
{
    g_running.store(false);
}

int main()
{
    using namespace rmq;

    const std::string exchange_name = "example_exchange";
    const std::string queue_name    = "example_queue";
    const std::string routing_key   = "example_key";

    // 注册信号处理
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

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

    // 4. 创建 Consumer
    Consumer consumer(pool);
    consumer.set_queue(queue_name)
        .set_timeout(1000)  // 设置1秒超时，使消费循环能及时响应退出信号
        .on_message(
            [](const amqp_envelope_t& envelope)
            {
                std::string body(static_cast<char*>(envelope.message.body.bytes),
                                 envelope.message.body.len);
                std::cout << "Received: " << body << std::endl;
            })
        .on_error([](const std::string& error)
                  { std::cerr << "Consumer error: " << error << std::endl; });

    consumer.start();
    std::cout << "Consumer started. Waiting for messages... (Ctrl+C to stop)" << std::endl;

    // 等待停止信号
    while (g_running.load())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Stopping consumer..." << std::endl;
    consumer.stop();
    consumer.join();

    std::cout << "Done." << std::endl;
    return 0;
}