#include "../../include/client/consumer.h"

#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

#include "../../include/broker/exchange.h"
#include "../../include/broker/queue.h"
#include "../../include/channnel/channelPool.h"
#include "../../include/connection/connection.h"

std::atomic<bool> g_running{true};

void signal_handler(int /*signum*/)
{
    g_running.store(false);
}

int main()
{
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    rmq::ConnConfig cfg;
    rmq::Connection conn;
    conn.init(cfg);

    rmq::ChannelPool pool(conn, 5);

    // 声明 topic 交换机
    rmq::ExchangeConfig ecfg;
    ecfg.name = "topic_logistics";
    ecfg.type = rmq::ExchangeType::Topic;
    rmq::Exchange exchange(ecfg);
    exchange.declare(*pool.acquire());

    // 创建临时队列并绑定到 "beijing.#"（所有北京的消息）
    rmq::QueueConfig qcfg;
    qcfg.name        = "";
    qcfg.auto_delete = true;
    qcfg.exclusive   = true;
    rmq::Queue q(qcfg);
    q.declare(*pool.acquire());
    q.bind(*pool.acquire(), "topic_logistics", "beijing.#");

    rmq::Consumer consumer(pool);
    consumer.set_queue(q.name())
        .on_message(
            [](const amqp_envelope_t& envelope)
            {
                std::string body(static_cast<char*>(envelope.message.body.bytes),
                                 envelope.message.body.len);
                std::cout << "[Beijing] recv: " << body << std::endl;
            })
        .on_error([](const std::string& str) { std::cout << "recv error: " << str << std::endl; });
    consumer.start();
    std::cout << "Beijing Consumer [" << q.name() << "] started. (Ctrl+C to stop)" << std::endl;

    // 等待停止信号
    while (g_running.load())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Stopping consumer..." << std::endl;
    consumer.stop();
    consumer.join();

    std::cout << "consumer Done." << std::endl;
    return 0;
}