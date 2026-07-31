#include "../include/cdc/communicator_factory.h"
#include "../include/cdc/protocol_strategy.h"
#include "../include/cdc/socket_ops.h"

namespace cdc
{

// ============================================================
// CommunicatorConfig 便捷工厂方法
// ============================================================

CommunicatorConfig CommunicatorConfig::tcpIpv4()
{
    return {AF_INET, SOCK_STREAM, 0};
}

CommunicatorConfig CommunicatorConfig::udpIpv4()
{
    return {AF_INET, SOCK_DGRAM, 0};
}

CommunicatorConfig CommunicatorConfig::tcpUnix()
{
    return {AF_UNIX, SOCK_STREAM, 0};
}

CommunicatorConfig CommunicatorConfig::udpUnix()
{
    return {AF_UNIX, SOCK_DGRAM, 0};
}

// ============================================================
// CommunicatorFactory 实现
// ============================================================

Communicator CommunicatorFactory::create(const CommunicatorConfig& config)
{
    // 1. 创建套接字
    int fd = SocketOps::socket(config.domain, config.type, config.protocol);
    if (fd < 0)
    {
        return Communicator();  // 返回无效通信器
    }

    // 2. 根据套接字类型选择策略
    std::unique_ptr<ProtocolStrategy> strategy;
    if (config.type == SOCK_STREAM)
    {
        strategy = std::make_unique<TcpStrategy>();
    }
    else if (config.type == SOCK_DGRAM)
    {
        strategy = std::make_unique<UdpStrategy>();
    }
    else
    {
        SocketOps::close(fd);
        return Communicator();
    }

    // 3. 组装 Communicator
    return Communicator(fd, config.domain, config.type, config.protocol,
                        std::move(strategy));
}

}  // namespace cdc