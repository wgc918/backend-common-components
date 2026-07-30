#pragma once

#include <amqp.h>
#include <amqp_tcp_socket.h>

#include <cstdint>
#include <string>

namespace rmq
{
struct ConnConfig
{
    std::string host        = "127.0.0.1";
    int         port        = 5672;
    std::string user        = "zsr";
    std::string password    = "123456";
    std::string vhost       = "/";
    int         channel_max = 0;  // 使用服务器默认值
    int         frame_max   = 0;  // 使用服务器默认值
    int         heartbeat   = 0;  // 禁用心跳
};

class Connection
{
public:
    Connection();
    Connection(const Connection& other)            = delete;
    Connection& operator=(const Connection& other) = delete;
    Connection(Connection&& other);
    Connection& operator=(Connection&& other);
    ~Connection();

    bool init(const ConnConfig& cfg);
    amqp_connection_state_t connection() const;

private:
    amqp_connection_state_t m_conn;
    bool                    m_init;
};
}  // namespace rmq