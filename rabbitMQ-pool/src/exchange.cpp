#include "../include/broker/exchange.h"

#include "../include/core/types.h"

namespace rmq
{

Exchange::Exchange(ExchangeConfig cfg) : m_cfg(std::move(cfg))
{
}

bool Exchange::declare(Channel& channel) const
{
    return channel.exchange_declare(m_cfg.name, exchange_type_to_string(m_cfg.type),
                                    false, m_cfg.durable, m_cfg.auto_delete,
                                    m_cfg.internal, m_cfg.arguments);
}

bool Exchange::remove(Channel& channel, bool if_unused) const
{
    return channel.exchange_delete(m_cfg.name, if_unused);
}

const std::string& Exchange::name() const
{
    return m_cfg.name;
}

ExchangeType Exchange::type() const
{
    return m_cfg.type;
}

}  // namespace rmq