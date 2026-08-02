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

    // 声明 direct 交换机（精确匹配路由）
    rmq::ExchangeConfig ecfg;
    ecfg.name = "direct_logs";
    ecfg.type = rmq::ExchangeType::Direct;
    rmq::Exchange exchange(ecfg);
    exchange.declare(*pool.acquire());

    rmq::Producer p(pool);

    // 路由模式：消息根据 routing_key 精确匹配分发到对应队列
    const char* severities[] = {"error", "warning", "info"};
    const char* messages[]   = {
        "error: disk full",
        "warning: disk usage 80%",
        "info: service started",
        "error: connection refused",
        "warning: memory usage 85%",
        "info: user login",
        "error: timeout",
        "info: request processed",
        "warning: cpu usage 70%",
        "info: heartbeat ok"
    };

    for (int i = 0; i < 10; i++)
    {
        const char* severity = severities[i % 3];
        rmq::Message message(messages[i]);
        message.set_exchange("direct_logs");
        message.set_routing_key(severity);
        p.send(message);
        std::cout << "sent [" << severity << "]: " << message.body() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
    }

    std::cout << "send done" << std::endl;
    return 0;
}