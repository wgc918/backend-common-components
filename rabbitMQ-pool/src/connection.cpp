#include "../include/connection/connection.h"

#include <amqp.h>
#include <rabbitmq-c/framing.h>

#include <iostream>

namespace rmq
{

Connection::Connection() : m_conn(nullptr), m_init(false)
{
}

Connection::Connection(Connection&& other)
    : m_conn(other.m_conn), m_init(other.m_init), m_config(std::move(other.m_config))
{
    other.m_conn = nullptr;
    other.m_init = false;
}

Connection& Connection::operator=(Connection&& other)
{
    if (this == &other)
        return *this;

    m_conn       = other.m_conn;
    m_init       = other.m_init;
    m_config     = std::move(other.m_config);
    other.m_init = false;
    other.m_conn = nullptr;

    return *this;
}

Connection::~Connection()
{
    if (m_conn != nullptr && m_init)
    {
        amqp_connection_close(m_conn, AMQP_REPLY_SUCCESS);
        amqp_destroy_connection(m_conn);
    }
}

bool Connection::init(const ConnConfig& cfg)
{
    if (m_init)
        return true;

    std::lock_guard<std::mutex> lock(m_mutex);

    m_config = cfg;

    m_conn = amqp_new_connection();
    if (m_conn == nullptr)
    {
        std::cerr << "[Error] Failed to create connection" << std::endl;
        return false;
    }

    amqp_socket_t* socket = amqp_tcp_socket_new(m_conn);
    if (socket == nullptr)
    {
        std::cerr << "[Error] Failed to create socket" << std::endl;
        return false;
    }

    int rv = amqp_socket_open(socket, cfg.host.c_str(), cfg.port);
    if (rv != 0)
    {
        std::cerr << "[Error] Failed to create TCP connection" << std::endl;
        return false;
    }

    auto reply =
        amqp_login(m_conn, cfg.vhost.c_str(), cfg.channel_max, cfg.frame_max, cfg.heartbeat,
                   AMQP_SASL_METHOD_PLAIN, cfg.user.c_str(), cfg.password.c_str());
    if (reply.reply_type != AMQP_RESPONSE_NORMAL)
    {
        std::cerr << "[Error] Failed to login" << std::endl;
        return false;
    }

    m_init = true;
    return true;
}

bool Connection::reconnect()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // 关闭旧连接
    if (m_conn != nullptr)
    {
        if (m_init)
        {
            amqp_connection_close(m_conn, AMQP_REPLY_SUCCESS);
        }
        amqp_destroy_connection(m_conn);
        m_conn = nullptr;
    }
    m_init = false;

    // 重新建立连接
    m_conn = amqp_new_connection();
    if (m_conn == nullptr)
    {
        std::cerr << "[Error] Reconnect: Failed to create connection" << std::endl;
        return false;
    }

    amqp_socket_t* socket = amqp_tcp_socket_new(m_conn);
    if (socket == nullptr)
    {
        std::cerr << "[Error] Reconnect: Failed to create socket" << std::endl;
        return false;
    }

    int rv = amqp_socket_open(socket, m_config.host.c_str(), m_config.port);
    if (rv != 0)
    {
        std::cerr << "[Error] Reconnect: Failed to create TCP connection" << std::endl;
        return false;
    }

    auto reply = amqp_login(m_conn, m_config.vhost.c_str(), m_config.channel_max,
                            m_config.frame_max, m_config.heartbeat, AMQP_SASL_METHOD_PLAIN,
                            m_config.user.c_str(), m_config.password.c_str());
    if (reply.reply_type != AMQP_RESPONSE_NORMAL)
    {
        std::cerr << "[Error] Reconnect: Failed to login" << std::endl;
        return false;
    }

    m_init = true;
    return true;
}

bool Connection::is_connected() const
{
    return m_init && m_conn != nullptr;
}

amqp_connection_state_t Connection::connection() const
{
    if (!m_init)
    {
        std::cerr << "[Warning] Connection not initialized" << std::endl;
        return nullptr;
    }
    return m_conn;
}

std::mutex& Connection::mutex()
{
    return m_mutex;
}

}  // namespace rmq