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

    for (int i = 0; i < 10; i++)
    {
        rmq::Message message("hello word: " + std::to_string(i + 1));
        message.set_routing_key("myqueue");
        // 当交换机为默认时，routing_key 就代表队列名
        p.send(message);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "send done" << std::endl;
    return 0;
}