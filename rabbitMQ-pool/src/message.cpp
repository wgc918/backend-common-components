#include "../include/message/message.h"

#include <cstring>

namespace rmq
{

Message::Message(std::string body, std::string exchange, std::string routing_key)
    : m_body(std::move(body)),
      m_exchange(std::move(exchange)),
      m_routing_key(std::move(routing_key))
{
}

Message& Message::set_body(std::string body)
{
    m_body = std::move(body);
    return *this;
}

Message& Message::set_exchange(std::string exchange)
{
    m_exchange = std::move(exchange);
    return *this;
}

Message& Message::set_routing_key(std::string routing_key)
{
    m_routing_key = std::move(routing_key);
    return *this;
}

Message& Message::set_content_type(std::string type)
{
    m_content_type = std::move(type);
    return *this;
}

Message& Message::set_content_encoding(std::string encoding)
{
    m_content_encoding = std::move(encoding);
    return *this;
}

Message& Message::set_delivery_mode(uint8_t mode)
{
    m_delivery_mode = mode;
    return *this;
}

Message& Message::set_correlation_id(std::string id)
{
    m_correlation_id = std::move(id);
    return *this;
}

Message& Message::set_reply_to(std::string queue)
{
    m_reply_to = std::move(queue);
    return *this;
}

Message& Message::set_message_id(std::string id)
{
    m_message_id = std::move(id);
    return *this;
}

Message& Message::set_priority(uint8_t priority)
{
    m_priority = priority;
    return *this;
}

Message& Message::set_expiration(std::string expiration)
{
    m_expiration = std::move(expiration);
    return *this;
}

Message& Message::set_headers(const amqp_table_t& headers)
{
    m_headers = headers;
    return *this;
}

amqp_basic_properties_t Message::properties() const
{
    amqp_basic_properties_t props = {};
    props._flags                  = 0;

    if (!m_content_type.empty())
    {
        props._flags |= AMQP_BASIC_CONTENT_TYPE_FLAG;
        props.content_type = to_amqp_bytes(m_content_type);
    }
    if (!m_content_encoding.empty())
    {
        props._flags |= AMQP_BASIC_CONTENT_ENCODING_FLAG;
        props.content_encoding = to_amqp_bytes(m_content_encoding);
    }
    if (m_delivery_mode > 0)
    {
        props._flags |= AMQP_BASIC_DELIVERY_MODE_FLAG;
        props.delivery_mode = m_delivery_mode;
    }
    if (!m_correlation_id.empty())
    {
        props._flags |= AMQP_BASIC_CORRELATION_ID_FLAG;
        props.correlation_id = to_amqp_bytes(m_correlation_id);
    }
    if (!m_reply_to.empty())
    {
        props._flags |= AMQP_BASIC_REPLY_TO_FLAG;
        props.reply_to = to_amqp_bytes(m_reply_to);
    }
    if (!m_message_id.empty())
    {
        props._flags |= AMQP_BASIC_MESSAGE_ID_FLAG;
        props.message_id = to_amqp_bytes(m_message_id);
    }
    if (m_priority > 0)
    {
        props._flags |= AMQP_BASIC_PRIORITY_FLAG;
        props.priority = m_priority;
    }
    if (!m_expiration.empty())
    {
        props._flags |= AMQP_BASIC_EXPIRATION_FLAG;
        props.expiration = to_amqp_bytes(m_expiration);
    }
    if (m_headers.num_entries > 0)
    {
        props._flags |= AMQP_BASIC_HEADERS_FLAG;
        props.headers = m_headers;
    }

    return props;
}

amqp_bytes_t Message::body_bytes() const
{
    return to_amqp_bytes(m_body);
}

amqp_bytes_t Message::to_amqp_bytes(const std::string& s)
{
    amqp_bytes_t bytes;
    bytes.len   = s.size();
    bytes.bytes = const_cast<char*>(s.data());
    return bytes;
}

}  // namespace rmq