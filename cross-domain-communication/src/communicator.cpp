#include "../include/cdc/communicator.h"
#include "../include/cdc/address.h"
#include "../include/cdc/socket_ops.h"

#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

namespace cdc
{

// ============================================================
// 构造函数与析构函数
// ============================================================

Communicator::Communicator()
    : fd_(-1), domain_(0), type_(0), protocol_(0), strategy_(nullptr)
{
}

Communicator::Communicator(int fd, int domain, int type, int protocol,
                           std::unique_ptr<ProtocolStrategy> strategy)
    : fd_(fd), domain_(domain), type_(type), protocol_(protocol),
      strategy_(std::move(strategy))
{
}

Communicator::Communicator(Communicator&& other) noexcept
    : fd_(other.fd_),
      domain_(other.domain_),
      type_(other.type_),
      protocol_(other.protocol_),
      strategy_(std::move(other.strategy_))
{
    other.fd_       = -1;
    other.domain_   = 0;
    other.type_     = 0;
    other.protocol_ = 0;
}

Communicator& Communicator::operator=(Communicator&& other) noexcept
{
    if (this != &other)
    {
        close();

        fd_        = other.fd_;
        domain_    = other.domain_;
        type_      = other.type_;
        protocol_  = other.protocol_;
        strategy_  = std::move(other.strategy_);

        other.fd_       = -1;
        other.domain_   = 0;
        other.type_     = 0;
        other.protocol_ = 0;
    }
    return *this;
}

Communicator::~Communicator()
{
    close();
}

// ============================================================
// 连接与绑定
// ============================================================

bool Communicator::connect(const Address& addr)
{
    if (fd_ < 0)
        return false;

    socklen_t addr_len = 0;
    const sockaddr* sa = addr.getSockAddr(addr_len);
    if (sa == nullptr)
        return false;

    return SocketOps::connect(fd_, sa, addr_len) == 0;
}

bool Communicator::bind(const Address& addr)
{
    if (fd_ < 0)
        return false;

    socklen_t addr_len = 0;
    const sockaddr* sa = addr.getSockAddr(addr_len);
    if (sa == nullptr)
        return false;

    return SocketOps::bind(fd_, sa, addr_len) == 0;
}

// ============================================================
// TCP 操作
// ============================================================

bool Communicator::listen(int backlog)
{
    if (fd_ < 0 || strategy_ == nullptr)
        return false;

    return strategy_->listen(this, backlog) == 0;
}

std::unique_ptr<Communicator> Communicator::accept()
{
    if (fd_ < 0 || strategy_ == nullptr)
        return nullptr;

    return strategy_->accept(this);
}

// ============================================================
// 数据收发
// ============================================================

ssize_t Communicator::send(const void* data, size_t len)
{
    if (fd_ < 0 || strategy_ == nullptr)
    {
        errno = EBADF;
        return -1;
    }

    return strategy_->send(this, data, len);
}

ssize_t Communicator::recv(void* buf, size_t len)
{
    if (fd_ < 0 || strategy_ == nullptr)
    {
        errno = EBADF;
        return -1;
    }

    return strategy_->recv(this, buf, len);
}

ssize_t Communicator::sendTo(const Address& addr, const void* data, size_t len)
{
    if (fd_ < 0 || strategy_ == nullptr)
    {
        errno = EBADF;
        return -1;
    }

    return strategy_->sendTo(this, addr, data, len);
}

ssize_t Communicator::recvFrom(Address& addr, void* buf, size_t len)
{
    if (fd_ < 0 || strategy_ == nullptr)
    {
        errno = EBADF;
        return -1;
    }

    return strategy_->recvFrom(this, addr, buf, len);
}

// ============================================================
// 套接字选项
// ============================================================

bool Communicator::setNonBlocking(bool nonblock)
{
    if (fd_ < 0)
        return false;

    int flags = fcntl(fd_, F_GETFL, 0);
    if (flags < 0)
        return false;

    if (nonblock)
        flags |= O_NONBLOCK;
    else
        flags &= ~O_NONBLOCK;

    return fcntl(fd_, F_SETFL, flags) == 0;
}

bool Communicator::setSockOpt(int level, int optname, const void* optval, socklen_t optlen)
{
    if (fd_ < 0)
        return false;

    return SocketOps::setsockopt(fd_, level, optname, optval, optlen) == 0;
}

// ============================================================
// 地址查询
// ============================================================

std::unique_ptr<Address> Communicator::getLocalAddress() const
{
    if (fd_ < 0)
        return nullptr;

    sockaddr_storage addr_buf{};
    socklen_t        addr_len = sizeof(addr_buf);

    if (SocketOps::getsockname(fd_, reinterpret_cast<sockaddr*>(&addr_buf), &addr_len) != 0)
        return nullptr;

    if (domain_ == AF_INET)
    {
        auto* sa_in = reinterpret_cast<sockaddr_in*>(&addr_buf);
        char  ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sa_in->sin_addr, ip_str, sizeof(ip_str));
        return std::make_unique<InetAddress>(ip_str, ntohs(sa_in->sin_port));
    }
    else if (domain_ == AF_UNIX)
    {
        auto* sa_un = reinterpret_cast<sockaddr_un*>(&addr_buf);
        return std::make_unique<UnixAddress>(sa_un->sun_path);
    }

    return nullptr;
}

std::unique_ptr<Address> Communicator::getPeerAddress() const
{
    if (fd_ < 0)
        return nullptr;

    sockaddr_storage addr_buf{};
    socklen_t        addr_len = sizeof(addr_buf);

    if (SocketOps::getpeername(fd_, reinterpret_cast<sockaddr*>(&addr_buf), &addr_len) != 0)
        return nullptr;

    if (domain_ == AF_INET)
    {
        auto* sa_in = reinterpret_cast<sockaddr_in*>(&addr_buf);
        char  ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sa_in->sin_addr, ip_str, sizeof(ip_str));
        return std::make_unique<InetAddress>(ip_str, ntohs(sa_in->sin_port));
    }
    else if (domain_ == AF_UNIX)
    {
        auto* sa_un = reinterpret_cast<sockaddr_un*>(&addr_buf);
        return std::make_unique<UnixAddress>(sa_un->sun_path);
    }

    return nullptr;
}

// ============================================================
// 生命周期
// ============================================================

void Communicator::close()
{
    if (fd_ >= 0)
    {
        SocketOps::close(fd_);
        fd_ = -1;
    }
    strategy_.reset();
}

bool Communicator::isValid() const noexcept
{
    return fd_ >= 0;
}

int Communicator::fd() const noexcept
{
    return fd_;
}

int Communicator::domain() const noexcept
{
    return domain_;
}

int Communicator::type() const noexcept
{
    return type_;
}

}  // namespace cdc