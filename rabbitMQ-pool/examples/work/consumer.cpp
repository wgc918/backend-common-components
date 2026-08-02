#include "../../include/client/consumer.h"

#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "../../include/broker/queue.h"
#include "../../include/channnel/channelPool.h"
#include "../../include/connection/connection.h"

std::atomic<bool> g_running{true};

void signal_handler(int /*signum*/)
{
    g_running.store(false);
}

std::mutex cout_mtx;

int main()
{
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    rmq::ConnConfig cfg;
    rmq::Connection conn;
    conn.init(cfg);

    rmq::ChannelPool pool(conn, 5);
    rmq::QueueConfig qcfg;
    qcfg.name    = "work_queue";
    qcfg.durable = true;  // 持久化队列，防止任务丢失
    rmq::Queue q(qcfg);
    q.declare(*pool.acquire());

    std::vector<std::unique_ptr<rmq::Consumer>> consumers;
    for (int i = 0; i < 3; i++)
    {
        auto c = std::make_unique<rmq::Consumer>(pool);
        c->set_queue("work_queue")
            .set_no_ack(false)      // 开启手动确认模式
            .set_prefetch_count(1)  // 公平分发：每次只取一条，处理完再取
            .set_timeout(1000 * 5)  // 便于在队列中没有消息后超时退出
            .on_message(
                [](const amqp_envelope_t& envelope)
                {
                    std::string body(static_cast<char*>(envelope.message.body.bytes),
                                     envelope.message.body.len);
                    cout_mtx.lock();
                    std::cout << "recv: " << body << std::endl;
                    cout_mtx.unlock();
                    
                    // 模拟耗时处理（每个任务耗时不同，体现公平分发）
                    int delay = (body.back() % 3 + 1) * 500;
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));

                    cout_mtx.lock();
                    std::cout << "done: " << body << " (took " << delay << "ms)" << std::endl;
                    cout_mtx.unlock();
                })
            .on_error([](const std::string& str)
                      { std::cout << "recv error: " << str << std::endl; });
        consumers.push_back(std::move(c));
    }

    for (auto& c : consumers)
        c->start();

    std::cout << "All consumers started. Waiting for tasks... (Ctrl+C to stop)" << std::endl;

    // 等待停止信号
    while (g_running.load())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Stopping consumers..." << std::endl;
    for (auto& c : consumers)
    {
        c->stop();
        c->join();
    }

    std::cout << "consumers Done." << std::endl;
    return 0;
}