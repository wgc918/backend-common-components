#include "../include/cdc/protocol_strategy.h"
#include "../include/cdc/address.h"
#include "../include/cdc/communicator.h"
#include "../include/cdc/socket_ops.h"

#include <cerrno>
#include <cstring>

namespace cdc
{

// ============================================================
// TcpStrategy 实现
// ============================================================

int TcpStrategy::listen(Communicator* comm, int backlog)
{
    return SocketOps::listen(comm->fd(), backlog);
}

std::unique_ptr<Communicator> TcpStrategy::accept(Communicator* comm)
{
    sockaddr_storage addr_buf{};
    socklen_t        addr_len = sizeof(addr_buf);

    int new_fd = SocketOps::accept(comm->fd(), reinterpret_cast<sockaddr*>(&addr_buf), &addr_len);
    if (new_fd < 0)
    {
        return nullptr;
    }

    // 创建新 Communicator，使用相同的策略（TCP）
    // 注意：不能用 make_unique，因为 Communicator 构造函数是私有的
    auto new_strategy = std::make_unique<TcpStrategy>();
    return std::unique_ptr<Communicator>(new Communicator(new_fd, comm->domain(), comm->type(), 0,
                                                          std::move(new_strategy)));
}

ssize_t TcpStrategy::send(Communicator* comm, const void* data, size_t len)
{
    return SocketOps::send(comm->fd(), data, len, 0);
}

ssize_t TcpStrategy::recv(Communicator* comm, void* buf, size_t len)
{
    return SocketOps::recv(comm->fd(), buf, len, 0);
}

ssize_t TcpStrategy::sendTo(Communicator* /*comm*/, const Address& /*addr*/,
                            const void* /*data*/, size_t /*len*/)
{
    errno = EOPNOTSUPP;
    return -1;
}

ssize_t TcpStrategy::recvFrom(Communicator* /*comm*/, Address& /*addr*/,
                              void* /*buf*/, size_t /*len*/)
{
    errno = EOPNOTSUPP;
    return -1;
}

// ============================================================
// UdpStrategy 实现
// ============================================================

int UdpStrategy::listen(Communicator* /*comm*/, int /*backlog*/)
{
    errno = EOPNOTSUPP;
    return -1;
}

std::unique_ptr<Communicator> UdpStrategy::accept(Communicator* /*comm*/)
{
    return nullptr;
}

ssize_t UdpStrategy::send(Communicator* comm, const void* data, size_t len)
{
    return SocketOps::send(comm->fd(), data, len, 0);
}

ssize_t UdpStrategy::recv(Communicator* comm, void* buf, size_t len)
{
    return SocketOps::recv(comm->fd(), buf, len, 0);
}

ssize_t UdpStrategy::sendTo(Communicator* comm, const Address& addr,
                            const void* data, size_t len)
{
    socklen_t addr_len = 0;
    const sockaddr* sa = addr.getSockAddr(addr_len);
    if (sa == nullptr)
    {
        errno = EINVAL;
        return -1;
    }
    return SocketOps::sendto(comm->fd(), data, len, 0, sa, addr_len);
}

ssize_t UdpStrategy::recvFrom(Communicator* comm, Address& addr,
                              void* buf, size_t len)
{
    sockaddr_storage addr_buf{};
    socklen_t        addr_len = sizeof(addr_buf);

    ssize_t ret = SocketOps::recvfrom(comm->fd(), buf, len, 0,
                                      reinterpret_cast<sockaddr*>(&addr_buf), &addr_len);
    if (ret < 0)
    {
        return ret;
    }

    // 根据 domain 解析来源地址
    if (comm->domain() == AF_INET)
    {
        auto* sa_in = reinterpret_cast<sockaddr_in*>(&addr_buf);
        char  ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sa_in->sin_addr, ip_str, sizeof(ip_str));
        addr = InetAddress(ip_str, ntohs(sa_in->sin_port));
    }
    else if (comm->domain() == AF_UNIX)
    {
        auto* sa_un = reinterpret_cast<sockaddr_un*>(&addr_buf);
        // sun_path 以 null 结尾（或我们手动确保）
        addr = UnixAddress(sa_un->sun_path);
    }
    // 其他 domain 暂不处理，addr 保持原样

    return ret;
}

}  // namespace cdc