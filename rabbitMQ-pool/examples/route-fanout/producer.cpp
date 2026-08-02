#include "../../include/client/producer.h"

#include <chrono>
#include <iostream>
#include <thread>

#include "../../include/broker/exchange.h"
#include "../../include/channnel/channelPool.h"
#include "../../include/connection/connection.h"

int main()
{
    rmq::Connection conn;
    rmq::ConnConfig cfg;
    conn.init(cfg);

    rmq::ChannelPool pool(conn, 10);

    // 声明 fanout 交换机（广播模式）
    rmq::ExchangeConfig ecfg;
    ecfg.name = "fanout_logs";
    ecfg.type = rmq::ExchangeType::Fanout;
    rmq::Exchange exchange(ecfg);
    exchange.declare(*pool.acquire());

    rmq::Producer p(pool);

    // 发布/订阅模式：消息发送到 fanout 交换机，所有绑定队列都会收到副本
    for (int i = 0; i < 10; i++)
    {
        rmq::Message message("broadcast msg: " + std::to_string(i + 1));
        message.set_exchange("fanout_logs");
        // fanout 模式忽略 routing_key，所有绑定队列都会收到
        //message.set_routing_key("");
        p.send(message);
        std::cout << "sent: " << message.body() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "send done" << std::endl;
    return 0;
}