#pragma once

#include <amqp.h>
#include <amqp_tcp_socket.h>

#include <cstdint>
#include <mutex>
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
    bool reconnect();
    bool is_connected() const;

    amqp_connection_state_t connection() const;

    /// 返回内部互斥锁引用，供 Channel 层在操作前加锁
    std::mutex& mutex()
    {
        return m_mutex;
    }

private:
    amqp_connection_state_t m_conn;
    bool                    m_init;
    ConnConfig              m_config;
    mutable std::mutex      m_mutex;
};
}  // namespace rmq