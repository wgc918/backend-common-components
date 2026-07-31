//-----------------------------------------------------------------------------
// 版权所有 (C) 2026 backend-common-components 项目
//
// 文件: address.h
// 作者: wgc
// 创建日期: 2026年8月
//
// 描述:
//     地址抽象模块 — 屏蔽 AF_UNIX 路径和 AF_INET IP+Port 差异。
//     提供统一的 getSockAddr() / toString() 接口和工厂方法。
//
// 功能特性:
//     - Address 抽象基类：定义 getSockAddr() 和 toString() 纯虚接口
//     - UnixAddress：Unix 域套接字地址，存储路径字符串
//     - InetAddress：IPv4 网络地址，存储 IP 字符串和端口号
//     - 工厂方法 Address::create() 根据 domain 参数自动创建对应子类
//
// 许可证:
//     MIT License
//
//     版权所有 (c) 2026 wgc
//
//     特此免费授予获得本软件副本和相关文档文件（以下简称"软件"）的任何人以处理软件的权利，
//     包括但不限于使用、复制、修改、合并、出版、分发、再许可和/或出售软件副本，
//     以及允许软件适用者这样做，须在下列条件下：
//
//     上述版权声明和本许可声明应包含在软件的所有副本或实质性部分中。
//
//     软件按"原样"提供，不提供任何形式的明示或暗示的保证，
//     包括但不限于对适销性、特定用途适用性和非侵权性的保证。
//     在任何情况下，作者或版权持有人均不对任何索赔、损害或其他责任负责，
//     无论是在合同诉讼、侵权诉讼或其他诉讼中，
//     由于软件或软件的使用或其他交易产生的。
//-----------------------------------------------------------------------------

#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>

#include <cstdint>
#include <memory>
#include <string>

namespace cdc
{

/// @brief 地址抽象基类，屏蔽 AF_UNIX 路径和 AF_INET IP+Port 差异
/// @details 纯虚接口，派生类实现具体的地址族表示。
///          使用 thread_local 缓冲区保证 getSockAddr() 的线程安全。
class Address
{
public:
    virtual ~Address() = default;

    /// @brief 获取 sockaddr 结构指针及长度
    /// @param[out] addr_len 输出参数，返回 sockaddr 结构的实际长度
    /// @return 指向 sockaddr 的只读指针
    virtual const sockaddr* getSockAddr(socklen_t& addr_len) const = 0;

    /// @brief 返回地址的人类可读字符串表示
    /// @return 地址字符串
    virtual std::string toString() const = 0;

    /// @brief 工厂方法：根据 domain 和参数创建对应的 Address 子类
    /// @param domain 地址族 (AF_UNIX 或 AF_INET)
    /// @param param1 对于 AF_UNIX 为路径字符串，对于 AF_INET 为 IP 字符串
    /// @param port   对于 AF_INET 为端口号（主机字节序），AF_UNIX 时忽略
    /// @return 堆上分配的 Address 智能指针，失败返回 nullptr
    static std::unique_ptr<Address> create(int domain, const std::string& param1,
                                           uint16_t port = 0);
};

/// @brief Unix 域套接字地址 (AF_UNIX)
/// @details 内部存储 Unix 域路径字符串，每次调用 getSockAddr() 时填充 sockaddr_un 结构。
class UnixAddress : public Address
{
public:
    /// @brief 构造 Unix 域地址
    /// @param path Unix 域套接字路径（如 /tmp/myapp.sock）
    explicit UnixAddress(std::string path);

    const sockaddr* getSockAddr(socklen_t& addr_len) const override;
    std::string toString() const override;

private:
    std::string path_;
};

/// @brief IPv4 网络地址 (AF_INET)
/// @details 内部存储 IP 地址字符串和端口号，每次调用 getSockAddr() 时填充 sockaddr_in 结构。
class InetAddress : public Address
{
public:
    /// @brief 构造 IPv4 地址
    /// @param ip   IPv4 地址字符串（如 "127.0.0.1"）
    /// @param port 端口号（主机字节序）
    InetAddress(std::string ip, uint16_t port);

    const sockaddr* getSockAddr(socklen_t& addr_len) const override;
    std::string toString() const override;

    /// @brief 获取 IP 地址
    const std::string& ip() const noexcept;

    /// @brief 获取端口号
    uint16_t port() const noexcept;

private:
    std::string ip_;
    uint16_t    port_;
};

}  // namespace cdc