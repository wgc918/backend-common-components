#include "../include/cdc/address.h"

#include <cstring>

namespace cdc
{

// ============================================================
// UnixAddress 实现
// ============================================================

UnixAddress::UnixAddress(std::string path) : path_(std::move(path)) {}

const sockaddr* UnixAddress::getSockAddr(socklen_t& addr_len) const
{
    static thread_local sockaddr_un addr{};

    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;

    // 安全复制路径，确保不超过 sun_path 容量
    // sizeof(sun_path) 通常为 108 字节（Linux）
    std::strncpy(addr.sun_path, path_.c_str(), sizeof(addr.sun_path) - 1);
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

    // 计算实际地址长度：offsetof(sun_path) + 路径长度 + 1（null terminator）
    addr_len = static_cast<socklen_t>(offsetof(sockaddr_un, sun_path) +
                                      std::strlen(addr.sun_path) + 1);

    return reinterpret_cast<const sockaddr*>(&addr);
}

std::string UnixAddress::toString() const
{
    return "unix://" + path_;
}

// ============================================================
// InetAddress 实现
// ============================================================

InetAddress::InetAddress(std::string ip, uint16_t port) : ip_(std::move(ip)), port_(port) {}

const sockaddr* InetAddress::getSockAddr(socklen_t& addr_len) const
{
    static thread_local sockaddr_in addr{};

    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port_);

    // 将 IP 字符串转换为网络字节序的二进制地址
    if (inet_pton(AF_INET, ip_.c_str(), &addr.sin_addr) != 1)
    {
        // 转换失败：返回 nullptr 表示无效地址
        addr_len = 0;
        return nullptr;
    }

    addr_len = sizeof(sockaddr_in);
    return reinterpret_cast<const sockaddr*>(&addr);
}

std::string InetAddress::toString() const
{
    return ip_ + ":" + std::to_string(port_);
}

const std::string& InetAddress::ip() const noexcept
{
    return ip_;
}

uint16_t InetAddress::port() const noexcept
{
    return port_;
}

// ============================================================
// Address 工厂方法
// ============================================================

std::unique_ptr<Address> Address::create(int domain, const std::string& param1, uint16_t port)
{
    switch (domain)
    {
    case AF_UNIX:
        return std::make_unique<UnixAddress>(param1);
    case AF_INET:
        return std::make_unique<InetAddress>(param1, port);
    default:
        return nullptr;
    }
}

}  // namespace cdc