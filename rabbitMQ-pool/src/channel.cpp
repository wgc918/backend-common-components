#include "../include/channnel/channel.h"

#include <amqp.h>
#include <amqp_tcp_socket.h>
#include <bits/types/struct_timeval.h>

#include <iostream>
#include <mutex>

namespace rmq
{

Channel::Channel(Connection& conn, channel_id id)
    : m_connection(&conn), m_channel_id(id), m_open(false)
{
}

Channel::Channel(Channel&& other) noexcept
    : m_connection(other.m_connection), m_channel_id(other.m_channel_id), m_open(other.m_open)
{
    // 注：因为设计上要让所有channel共用一个connection,
    // 所以移动时不应该置空other.m_connection
    other.m_channel_id = 0;
    other.m_open       = false;
}

Channel& Channel::operator=(Channel&& other) noexcept
{
    if (this == &other)
        return *this;

    close();

    // 注：因为设计上要让所有channel共用一个connection,
    // 所以移动时不应该置空other.m_connection
    m_connection       = other.m_connection;
    m_channel_id       = other.m_channel_id;
    m_open             = other.m_open;
    other.m_channel_id = 0;
    other.m_open       = false;

    return *this;
}

Channel::~Channel()
{
    set_rpc_timeout(1000);
    close();
    set_rpc_timeout(120 * 1000);
}

bool Channel::open()
{
    amqp_connection_state_t conn = m_connection->connection();
    if (conn == nullptr)
    {
        std::cerr << "[Error] Channel::open: connection not initialized" << std::endl;
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_connection->mutex());
        amqp_channel_open(conn, m_channel_id);
        if (!check_rpc_reply("Opening channel"))
        {
            return false;
        }
    }

    m_open = true;
    return true;
}

bool Channel::close()
{
    if (!m_open)
        return true;

    amqp_connection_state_t conn = m_connection->connection();
    if (conn == nullptr)
    {
        std::cerr << "[Error] Channel::close: connection not initialized" << std::endl;
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_connection->mutex());
        amqp_channel_close(conn, m_channel_id, AMQP_REPLY_SUCCESS);
        if (!check_rpc_reply("Closing channel"))
        {
            return false;
        }
    }

    m_open = false;
    return true;
}

bool Channel::is_open() const
{
    return m_open;
}

channel_id Channel::id() const
{
    return m_channel_id;
}

bool Channel::exchange_declare(const std::string& name, const std::string& type, bool passive,
                               bool durable, bool auto_delete, bool internal,
                               amqp_table_t arguments)
{
    std::lock_guard<std::mutex> lock(m_connection->mutex());

    amqp_exchange_declare(m_connection->connection(), m_channel_id,
                          amqp_cstring_bytes(name.c_str()), amqp_cstring_bytes(type.c_str()),
                          passive ? 1 : 0, durable ? 1 : 0, auto_delete ? 1 : 0, internal ? 1 : 0,
                          arguments);
    return check_rpc_reply("Declaring exchange");
}

bool Channel::exchange_delete(const std::string& name, bool if_unused)
{
    std::lock_guard<std::mutex> lock(m_connection->mutex());

    amqp_exchange_delete(m_connection->connection(), m_channel_id, amqp_cstring_bytes(name.c_str()),
                         if_unused ? 1 : 0);
    return check_rpc_reply("Deleting exchange");
}

bool Channel::exchange_bind(const std::string& destination, const std::string& source,
                            const std::string& routing_key, amqp_table_t arguments)
{
    std::lock_guard<std::mutex> lock(m_connection->mutex());

    amqp_exchange_bind(m_connection->connection(), m_channel_id,
                       amqp_cstring_bytes(destination.c_str()), amqp_cstring_bytes(source.c_str()),
                       amqp_cstring_bytes(routing_key.c_str()), arguments);
    return check_rpc_reply("Binding exchange");
}

bool Channel::exchange_unbind(const std::string& destination, const std::string& source,
                              const std::string& routing_key, amqp_table_t arguments)
{
    std::lock_guard<std::mutex> lock(m_connection->mutex());

    amqp_exchange_unbind(
        m_connection->connection(), m_channel_id, amqp_cstring_bytes(destination.c_str()),
        amqp_cstring_bytes(source.c_str()), amqp_cstring_bytes(routing_key.c_str()), arguments);
    return check_rpc_reply("Unbinding exchange");
}

bool Channel::queue_declare(const std::string& name, bool passive, bool durable, bool exclusive,
                            bool auto_delete, amqp_table_t arguments)
{
    std::lock_guard<std::mutex> lock(m_connection->mutex());

    amqp_queue_declare(m_connection->connection(), m_channel_id, amqp_cstring_bytes(name.c_str()),
                       passive ? 1 : 0, durable ? 1 : 0, exclusive ? 1 : 0, auto_delete ? 1 : 0,
                       arguments);
    return check_rpc_reply("Declaring queue");
}

bool Channel::queue_delete(const std::string& name, bool if_unused, bool if_empty)
{
    std::lock_guard<std::mutex> lock(m_connection->mutex());

    amqp_queue_delete(m_connection->connection(), m_channel_id, amqp_cstring_bytes(name.c_str()),
                      if_unused ? 1 : 0, if_empty ? 1 : 0);
    return check_rpc_reply("Deleting queue");
}

bool Channel::queue_bind(const std::string& queue, const std::string& exchange,
                         const std::string& routing_key, amqp_table_t arguments)
{
    std::lock_guard<std::mutex> lock(m_connection->mutex());

    amqp_queue_bind(m_connection->connection(), m_channel_id, amqp_cstring_bytes(queue.c_str()),
                    amqp_cstring_bytes(exchange.c_str()), amqp_cstring_bytes(routing_key.c_str()),
                    arguments);
    return check_rpc_reply("Binding queue");
}

bool Channel::queue_unbind(const std::string& queue, const std::string& exchange,
                           const std::string& routing_key, amqp_table_t arguments)
{
    std::lock_guard<std::mutex> lock(m_connection->mutex());

    amqp_queue_unbind(m_connection->connection(), m_channel_id, amqp_cstring_bytes(queue.c_str()),
                      amqp_cstring_bytes(exchange.c_str()), amqp_cstring_bytes(routing_key.c_str()),
                      arguments);
    return check_rpc_reply("Unbinding queue");
}

bool Channel::queue_purge(const std::string& queue)
{
    std::lock_guard<std::mutex> lock(m_connection->mutex());

    amqp_queue_purge(m_connection->connection(), m_channel_id, amqp_cstring_bytes(queue.c_str()));
    return check_rpc_reply("Purging queue");
}

bool Channel::basic_publish(const std::string& exchange, const std::string& routing_key,
                            const amqp_basic_properties_t* props, const amqp_bytes_t& body,
                            bool mandatory, bool immediate)
{
    int result;
    {
        std::lock_guard<std::mutex> lock(m_connection->mutex());

        result = amqp_basic_publish(m_connection->connection(), m_channel_id,
                                    amqp_cstring_bytes(exchange.c_str()),
                                    amqp_cstring_bytes(routing_key.c_str()), mandatory ? 1 : 0,
                                    immediate ? 1 : 0, props, body);
    }

    if (result != AMQP_STATUS_OK)
    {
        std::cerr << "[Error] Publishing message: " << amqp_error_string2(result) << std::endl;
        return false;
    }
    return true;
}

bool Channel::basic_consume(const std::string& queue, const std::string& consumer_tag,
                            bool no_local, bool no_ack, bool exclusive, amqp_table_t arguments)
{
    std::lock_guard<std::mutex> lock(m_connection->mutex());

    amqp_bytes_t tag_bytes =
        consumer_tag.empty() ? amqp_empty_bytes : amqp_cstring_bytes(consumer_tag.c_str());

    amqp_basic_consume(m_connection->connection(), m_channel_id, amqp_cstring_bytes(queue.c_str()),
                       tag_bytes, no_local ? 1 : 0, no_ack ? 1 : 0, exclusive ? 1 : 0, arguments);
    return check_rpc_reply("Starting consume");
}

bool Channel::basic_ack(uint64_t delivery_tag, bool multiple)
{
    std::lock_guard<std::mutex> lock(m_connection->mutex());

    amqp_basic_ack(m_connection->connection(), m_channel_id, delivery_tag, multiple ? 1 : 0);
    // basic_ack 是异步的，不检查 RPC 回复
    return true;
}

bool Channel::basic_nack(uint64_t delivery_tag, bool multiple, bool requeue)
{
    std::lock_guard<std::mutex> lock(m_connection->mutex());

    amqp_basic_nack(m_connection->connection(), m_channel_id, delivery_tag, multiple ? 1 : 0,
                    requeue ? 1 : 0);
    return true;
}

bool Channel::basic_qos(uint32_t prefetch_size, uint16_t prefetch_count, bool global)
{
    std::lock_guard<std::mutex> lock(m_connection->mutex());

    amqp_basic_qos(m_connection->connection(), m_channel_id, prefetch_size, prefetch_count,
                   global ? 1 : 0);
    return check_rpc_reply("Setting QoS");
}

bool Channel::basic_cancel(const std::string& consumer_tag)
{
    std::lock_guard<std::mutex> lock(m_connection->mutex());

    amqp_basic_cancel(m_connection->connection(), m_channel_id,
                      amqp_cstring_bytes(consumer_tag.c_str()));
    return check_rpc_reply("Canceling consumer");
}

void Channel::set_rpc_timeout(int timeout_ms)
{
    // 默认是120s
    if (timeout_ms < 0)
        return;

    timeval tv;
    tv.tv_sec  = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    std::lock_guard<std::mutex> lock(m_connection->mutex());
    amqp_set_rpc_timeout(m_connection->connection(), &tv);
}

bool Channel::consume_message(amqp_envelope_t& envelope, int timeout_ms)
{
    timeval  tv;
    timeval* tv_ptr = nullptr;
    if (timeout_ms >= 0)
    {
        tv.tv_sec  = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        tv_ptr     = &tv;
    }

    amqp_rpc_reply_t res;
    {
        std::lock_guard<std::mutex> lock(m_connection->mutex());

        amqp_maybe_release_buffers(m_connection->connection());

        res = amqp_consume_message(m_connection->connection(), &envelope, tv_ptr, 0);
    }

    if (res.reply_type != AMQP_RESPONSE_NORMAL)
    {
        return false;
    }
    return true;
}

bool Channel::check_rpc_reply(const char* context)
{
    amqp_rpc_reply_t reply = amqp_get_rpc_reply(m_connection->connection());

    if (reply.reply_type != AMQP_RESPONSE_NORMAL)
    {
        if (reply.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION)
        {
            std::cerr << "[Error] " << context << ": " << amqp_error_string2(reply.library_error)
                      << std::endl;
        }
        else if (reply.reply_type == AMQP_RESPONSE_SERVER_EXCEPTION)
        {
            std::cerr << "[Error] " << context << ": " << ": server exception - " << std::endl;
        }
        return false;
    }
    return true;
}

Connection& Channel::connection()
{
    return *m_connection;
}

}  // namespace rmq