#include "../include/cdc/socket_ops.h"

#include <unistd.h>  // ::close

namespace cdc
{

int SocketOps::socket(int domain, int type, int protocol) noexcept
{
    return ::socket(domain, type, protocol);
}

int SocketOps::bind(int fd, const sockaddr* addr, socklen_t len) noexcept
{
    return ::bind(fd, addr, len);
}

int SocketOps::connect(int fd, const sockaddr* addr, socklen_t len) noexcept
{
    return ::connect(fd, addr, len);
}

int SocketOps::listen(int fd, int backlog) noexcept
{
    return ::listen(fd, backlog);
}

int SocketOps::accept(int fd, sockaddr* addr, socklen_t* addr_len) noexcept
{
    return ::accept(fd, addr, addr_len);
}

ssize_t SocketOps::send(int fd, const void* data, size_t len, int flags) noexcept
{
    return ::send(fd, data, len, flags);
}

ssize_t SocketOps::recv(int fd, void* buf, size_t len, int flags) noexcept
{
    return ::recv(fd, buf, len, flags);
}

ssize_t SocketOps::sendto(int fd, const void* data, size_t len, int flags,
                          const sockaddr* addr, socklen_t addr_len) noexcept
{
    return ::sendto(fd, data, len, flags, addr, addr_len);
}

ssize_t SocketOps::recvfrom(int fd, void* buf, size_t len, int flags,
                            sockaddr* addr, socklen_t* addr_len) noexcept
{
    return ::recvfrom(fd, buf, len, flags, addr, addr_len);
}

int SocketOps::getsockname(int fd, sockaddr* addr, socklen_t* addr_len) noexcept
{
    return ::getsockname(fd, addr, addr_len);
}

int SocketOps::getpeername(int fd, sockaddr* addr, socklen_t* addr_len) noexcept
{
    return ::getpeername(fd, addr, addr_len);
}

int SocketOps::setsockopt(int fd, int level, int optname,
                          const void* optval, socklen_t optlen) noexcept
{
    return ::setsockopt(fd, level, optname, optval, optlen);
}

int SocketOps::close(int fd) noexcept
{
    return ::close(fd);
}

}  // namespace cdc