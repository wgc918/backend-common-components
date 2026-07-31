//-----------------------------------------------------------------------------
// 版权所有 (C) 2026 backend-common-components 项目
//
// 文件: protocol_strategy.h
// 作者: wgc
// 创建日期: 2026年8月
//
// 描述:
//     协议策略模块 — 定义协议无关的通信操作接口，
//     由 TcpStrategy 和 UdpStrategy 分别实现 TCP/UDP 语义。
//
// 功能特性:
//     - ProtocolStrategy 抽象基类：定义 listen/accept/send/recv/sendTo/recvFrom 接口
//     - TcpStrategy：TCP 流式协议语义，sendTo/recvFrom 返回 EOPNOTSUPP
//     - UdpStrategy：UDP 数据报语义，listen/accept 返回 EOPNOTSUPP
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

#include <sys/types.h>

#include <cstddef>
#include <memory>

namespace cdc
{

// 前向声明，避免循环依赖
class Communicator;
class Address;

/// @brief 协议策略抽象基类
/// @details 定义协议无关的通信操作接口，由具体策略实现 TCP/UDP 语义。
///          策略通过 Communicator 指针访问套接字描述符。
class ProtocolStrategy
{
public:
    virtual ~ProtocolStrategy() = default;

    /// @brief 监听（仅 TCP 有效）
    /// @param comm    通信器指针
    /// @param backlog 未完成连接队列最大长度
    /// @return 成功返回 0，失败返回 -1（UDP 调用返回 -1 并设置 errno = EOPNOTSUPP）
    virtual int listen(Communicator* comm, int backlog) = 0;

    /// @brief 接受连接（仅 TCP 有效）
    /// @param comm 通信器指针
    /// @return 成功返回新的 Communicator 智能指针，失败返回 nullptr
    virtual std::unique_ptr<Communicator> accept(Communicator* comm) = 0;

    /// @brief 发送数据
    /// @param comm 通信器指针
    /// @param data 数据缓冲区
    /// @param len  数据长度
    /// @return 成功返回发送字节数，失败返回 -1
    virtual ssize_t send(Communicator* comm, const void* data, size_t len) = 0;

    /// @brief 接收数据
    /// @param comm 通信器指针
    /// @param buf  接收缓冲区
    /// @param len  缓冲区大小
    /// @return 成功返回接收字节数，失败返回 -1
    virtual ssize_t recv(Communicator* comm, void* buf, size_t len) = 0;

    /// @brief 向指定地址发送数据（仅 UDP 有效）
    /// @param comm 通信器指针
    /// @param addr 目标地址
    /// @param data 数据缓冲区
    /// @param len  数据长度
    /// @return 成功返回发送字节数，失败返回 -1
    virtual ssize_t sendTo(Communicator* comm, const Address& addr,
                           const void* data, size_t len) = 0;

    /// @brief 从任意地址接收数据（仅 UDP 有效）
    /// @param comm 通信器指针
    /// @param addr 输出参数，来源地址
    /// @param buf  接收缓冲区
    /// @param len  缓冲区大小
    /// @return 成功返回接收字节数，失败返回 -1
    virtual ssize_t recvFrom(Communicator* comm, Address& addr,
                             void* buf, size_t len) = 0;
};

/// @brief TCP 流式协议策略
/// @details 实现 TCP 流语义：面向连接、可靠传输、字节流。
///          sendTo / recvFrom 不支持，调用返回 -1 并设置 errno = EOPNOTSUPP。
class TcpStrategy : public ProtocolStrategy
{
public:
    int listen(Communicator* comm, int backlog) override;
    std::unique_ptr<Communicator> accept(Communicator* comm) override;
    ssize_t send(Communicator* comm, const void* data, size_t len) override;
    ssize_t recv(Communicator* comm, void* buf, size_t len) override;

    /// @brief TCP 不支持 sendTo，调用返回 -1 并设置 errno = EOPNOTSUPP
    ssize_t sendTo(Communicator* comm, const Address& addr,
                   const void* data, size_t len) override;

    /// @brief TCP 不支持 recvFrom，调用返回 -1 并设置 errno = EOPNOTSUPP
    ssize_t recvFrom(Communicator* comm, Address& addr,
                     void* buf, size_t len) override;
};

/// @brief UDP 数据报协议策略
/// @details 实现 UDP 数据报语义：无连接、不可靠、保留消息边界。
///          listen / accept 不支持，调用返回 -1 或 nullptr。
class UdpStrategy : public ProtocolStrategy
{
public:
    /// @brief UDP 不支持 listen，调用返回 -1 并设置 errno = EOPNOTSUPP
    int listen(Communicator* comm, int backlog) override;

    /// @brief UDP 不支持 accept，调用返回 nullptr
    std::unique_ptr<Communicator> accept(Communicator* comm) override;

    /// @brief UDP send：若已 connect 则发送到默认对端，否则返回 -1
    ssize_t send(Communicator* comm, const void* data, size_t len) override;

    /// @brief UDP recv：若已 connect 则从默认对端接收，否则返回 -1
    ssize_t recv(Communicator* comm, void* buf, size_t len) override;

    /// @brief UDP sendTo：发送到指定地址
    ssize_t sendTo(Communicator* comm, const Address& addr,
                   const void* data, size_t len) override;

    /// @brief UDP recvFrom：接收并获取来源地址
    ssize_t recvFrom(Communicator* comm, Address& addr,
                     void* buf, size_t len) override;
};

}  // namespace cdc