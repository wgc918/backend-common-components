#include <amqp.h>
#include <amqp_tcp_socket.h>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

// 用于处理 AMQP 错误并输出错误信息
void die_on_error(amqp_rpc_reply_t x, const char* context)
{
    if (x.reply_type != AMQP_RESPONSE_NORMAL)
    {
        std::cerr << "Error in " << context << ": " << amqp_error_string2(x.library_error)
                  << std::endl;
        exit(1);
    }
}

int main()
{
    const std::string hostname    = "localhost";         // RabbitMQ 服务器地址
    const int         port        = 5672;                // RabbitMQ 默认端口
    const std::string exchange    = "example_exchange";  // 交换机名称
    const std::string routing_key = "example_key";       // 路由键，用于绑定队列

    // 初始化连接
    amqp_connection_state_t conn   = amqp_new_connection();
    amqp_socket_t*          socket = amqp_tcp_socket_new(conn);
    if (!socket)
    {
        std::cerr << "Creating TCP socket failed" << std::endl;
        return 1;
    }

    // 打开 TCP 连接
    int status = amqp_socket_open(socket, hostname.c_str(), port);
    if (status)
    {
        std::cerr << "Opening TCP socket failed" << std::endl;
        return 1;
    }

    // 登录 RabbitMQ
    die_on_error(amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "zsr", "123456"),
                 "Logging in");
    amqp_channel_open(conn, 1);  // 打开信道
    die_on_error(amqp_get_rpc_reply(conn), "Opening channel");

    // 声明交换机（类型为 direct）
    amqp_exchange_declare(conn, 1, amqp_cstring_bytes(exchange.c_str()),
                          amqp_cstring_bytes("direct"), 0, 0, 0, 0, amqp_empty_table);
    die_on_error(amqp_get_rpc_reply(conn), "Declaring exchange");

    // 循环发送多条消息
    for (int i = 1; i <= 1000; ++i)
    {  // 发送 1000 条消息
        std::string  message       = "Hello, RabbitMQ! Message number: " + std::to_string(i);
        amqp_bytes_t message_bytes = amqp_cstring_bytes(message.c_str());

        // 设置消息属性
        amqp_basic_properties_t props;
        props._flags        = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
        props.content_type  = amqp_cstring_bytes("text/plain");
        props.delivery_mode = 2;  // 持久化模式

        // 发送消息到交换机
        int result = amqp_basic_publish(conn, 1, amqp_cstring_bytes(exchange.c_str()),
                                        amqp_cstring_bytes(routing_key.c_str()), 0, 0, &props,
                                        message_bytes);
        if (result < 0)
        {
            std::cerr << "Error publishing message " << i << std::endl;
        }
        else
        {
            std::cout << "Message " << i << " published: " << message << std::endl;
        }

        // 每次发送后等待 1 秒
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // 清理连接
    amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS);
    amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
    amqp_destroy_connection(conn);

    return 0;
}