#include <amqp.h>
#include <amqp_tcp_socket.h>

#include <iostream>
#include <string>

// 错误处理函数，用于输出错误信息
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
    const int         port        = 5672;                // 端口
    const std::string queue       = "example_queue";     // 队列名称
    const std::string exchange    = "example_exchange";  // 交换机名称
    const std::string routing_key = "example_key";       // 路由键

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
    amqp_channel_open(conn, 1);
    die_on_error(amqp_get_rpc_reply(conn), "Opening channel");

    // 声明交换机和队列，并绑定队列到交换机
    amqp_exchange_declare(conn, 1, amqp_cstring_bytes(exchange.c_str()),
                          amqp_cstring_bytes("direct"), 0, 0, 0, 0, amqp_empty_table);
    die_on_error(amqp_get_rpc_reply(conn), "Declaring exchange");

    amqp_queue_declare_ok_t* q = amqp_queue_declare(conn, 1, amqp_cstring_bytes(queue.c_str()), 0,
                                                    0, 0, 1, amqp_empty_table);
    die_on_error(amqp_get_rpc_reply(conn), "Declaring queue");

    amqp_queue_bind(conn, 1, amqp_cstring_bytes(queue.c_str()),
                    amqp_cstring_bytes(exchange.c_str()), amqp_cstring_bytes(routing_key.c_str()),
                    amqp_empty_table);
    die_on_error(amqp_get_rpc_reply(conn), "Binding queue");

    // 开始消费消息
    amqp_basic_consume(conn, 1, amqp_cstring_bytes(queue.c_str()), amqp_empty_bytes, 0, 1, 0,
                       amqp_empty_table);
    die_on_error(amqp_get_rpc_reply(conn), "Consuming");

    while (true)
    {
        amqp_rpc_reply_t res;
        amqp_envelope_t  envelope;

        // 释放资源
        amqp_maybe_release_buffers(conn);
        res = amqp_consume_message(conn, &envelope, NULL, 0);

        // 检查并打印接收到的消息
        if (res.reply_type == AMQP_RESPONSE_NORMAL)
        {
            std::cout << "Received: "
                      << std::string((char*)envelope.message.body.bytes, envelope.message.body.len)
                      << std::endl;
            amqp_destroy_envelope(&envelope);
        }
        else
        {
            std::cerr << "Error consuming message" << std::endl;
            break;
        }
    }
}