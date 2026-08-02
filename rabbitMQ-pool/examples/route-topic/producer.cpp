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

    // 声明 topic 交换机（通配符匹配路由）
    rmq::ExchangeConfig ecfg;
    ecfg.name = "topic_logistics";
    ecfg.type = rmq::ExchangeType::Topic;
    rmq::Exchange exchange(ecfg);
    exchange.declare(*pool.acquire());

    rmq::Producer p(pool);

    // 主题模式：routing_key 使用 <城市>.<区域>.<状态> 格式
    // 支持通配符：* 匹配一个单词，# 匹配零个或多个单词
    struct LogisticsMsg
    {
        const char* routing_key;
        const char* body;
    };

    const LogisticsMsg messages[] = {
        {"beijing.chaoyang.delivering", "package #1001 in 朝阳区, delivering"},
        {"beijing.haidian.arrived", "package #1002 in 海淀区, arrived"},
        {"shanghai.pudong.delivering", "package #2001 in 浦东新区, delivering"},
        {"beijing.chaoyang.delivered", "package #1003 in 朝阳区, delivered"},
        {"shenzhen.nanshan.delivering", "package #3001 in 南山区, delivering"},
        {"shanghai.jingan.arrived", "package #2002 in 静安区, arrived"},
        {"beijing.xicheng.delivering", "package #1004 in 西城区, delivering"},
        {"shanghai.pudong.delivered", "package #2003 in 浦东新区, delivered"},
    };

    for (int i = 0; i < 8; i++)
    {
        rmq::Message message(messages[i].body);
        message.set_exchange("topic_logistics");
        message.set_routing_key(messages[i].routing_key);
        p.send(message);
        std::cout << "sent [" << messages[i].routing_key << "]: " << message.body() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
    }

    std::cout << "send done" << std::endl;
    return 0;
}