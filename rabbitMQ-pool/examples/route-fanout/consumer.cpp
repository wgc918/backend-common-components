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

    // 声明 fanout 交换机
    rmq::ExchangeConfig ecfg;
    ecfg.name = "fanout_logs";
    ecfg.type = rmq::ExchangeType::Fanout;
    rmq::Exchange exchange(ecfg);
    exchange.declare(*pool.acquire());

    // 声明一个独占的临时队列（名称由服务器自动生成）
    rmq::QueueConfig qcfg;
    qcfg.name        = "";
    qcfg.auto_delete = true;
    qcfg.exclusive   = true;
    rmq::Queue q(qcfg);
    q.declare(*pool.acquire());

    // 绑定到 fanout 交换机
    q.bind(*pool.acquire(), "fanout_logs", "");

    rmq::Consumer consumer(pool);
    consumer.set_queue(q.name())
        .on_message(
            [](const amqp_envelope_t& envelope)
            {
                std::string body(static_cast<char*>(envelope.message.body.bytes),
                                 envelope.message.body.len);
                std::cout << "recv: " << body << std::endl;
            })
        .on_error([](const std::string& str) { std::cout << "recv error: " << str << std::endl; });
    consumer.start();
    std::cout << "Consumer [" << q.name() << "] started. (Ctrl+C to stop)" << std::endl;

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