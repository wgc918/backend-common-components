#include "../../include/client/producer.h"

#include <chrono>
#include <iostream>
#include <thread>

#include "../../include/channnel/channelPool.h"
#include "../../include/connection/connection.h"

int main()
{
    rmq::Connection conn;
    rmq::ConnConfig cfg;
    conn.init(cfg);

    rmq::ChannelPool pool(conn, 10);
    rmq::Producer    p(pool);

    // 工作模式：发送大量耗时任务到同一个队列，多个消费者竞争消费
    for (int i = 0; i < 20; i++)
    {
        rmq::Message message("task: " + std::to_string(i + 1));
        // 使用默认交换机，routing_key 就是队列名
        message.set_routing_key("work_queue");
        p.send(message);
        std::cout << "sent task " << (i + 1) << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "send done" << std::endl;
    return 0;
}