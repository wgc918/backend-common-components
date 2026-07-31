//-----------------------------------------------------------------------------
// 版权所有 (C) 2026 backend-common-components 项目
//
// 文件: communicator.h
// 作者: wgc
// 创建日期: 2026年8月
//
// 描述:
//     通信器核心类 — 封装套接字操作并提供统一通信接口。
//     内部根据协议类型（TCP/UDP）将操作委托给策略对象。
//
// 功能特性:
//     - 可移动不可拷贝，持有套接字描述符和协议策略
//     - connect / bind / listen / accept 连接管理
//     - send / recv / sendTo / recvFrom 数据收发
//     - getLocalAddress / getPeerAddress 地址查询
//     - setNonBlocking / setSockOpt 套接字选项
//     - 析构自动 close，RAII 资源管理
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

#include "address.h"
#include "protocol_strategy.h"

#include <sys/socket.h>

#include <memory>

namespace cdc
{

/// @brief 通信器核心类，封装套接字操作并提供统一通信接口
/// @details 可移动但不可拷贝。内部持有套接字描述符和协议策略，
///          根据协议类型（TCP/UDP）将操作委托给策略对象。
class Communicator
{
    friend class CommunicatorFactory;  // 工厂可访问私有构造函数
    friend class TcpStrategy;          // 策略需要访问 fd() 等
    friend class UdpStrategy;

public:
    // ---- 生命周期 ----

    /// @brief 默认构造，创建无效通信器（fd_ = -1）
    Communicator();

    /// @brief 析构，自动关闭套接字
    ~Communicator();

    /// @brief 移动构造
    Communicator(Communicator&& other) noexcept;

    /// @brief 移动赋值
    Communicator& operator=(Communicator&& other) noexcept;

    // 禁止拷贝
    Communicator(const Communicator&)            = delete;
    Communicator& operator=(const Communicator&) = delete;

    // ---- 连接与绑定 ----

    /// @brief 连接到远程地址
    /// @param addr 目标地址
    /// @return 成功返回 true，失败返回 false
    bool connect(const Address& addr);

    /// @brief 绑定到本地地址
    /// @param addr 本地地址
    /// @return 成功返回 true，失败返回 false
    bool bind(const Address& addr);

    // ---- TCP 操作 ----

    /// @brief 开始监听（仅 TCP 有效）
    /// @param backlog 未完成连接队列最大长度
    /// @return 成功返回 true，失败返回 false
    bool listen(int backlog = 128);

    /// @brief 接受连接（仅 TCP 有效）
    /// @return 成功返回新 Communicator 智能指针，失败返回 nullptr
    std::unique_ptr<Communicator> accept();

    // ---- 数据收发 ----

    /// @brief 发送数据（TCP 直接发送，UDP 若已 connect 则发送到默认对端）
    /// @param data 数据缓冲区
    /// @param len  数据长度
    /// @return 成功返回发送字节数，失败返回 -1
    ssize_t send(const void* data, size_t len);

    /// @brief 接收数据（TCP 直接接收，UDP 若已 connect 则从默认对端接收）
    /// @param buf 接收缓冲区
    /// @param len 缓冲区大小
    /// @return 成功返回接收字节数，失败返回 -1
    ssize_t recv(void* buf, size_t len);

    /// @brief 向指定地址发送数据（仅 UDP 有效）
    /// @param addr 目标地址
    /// @param data 数据缓冲区
    /// @param len  数据长度
    /// @return 成功返回发送字节数，失败返回 -1
    ssize_t sendTo(const Address& addr, const void* data, size_t len);

    /// @brief 从任意地址接收数据（仅 UDP 有效）
    /// @param addr 输出参数，来源地址
    /// @param buf  接收缓冲区
    /// @param len  缓冲区大小
    /// @return 成功返回接收字节数，失败返回 -1
    ssize_t recvFrom(Address& addr, void* buf, size_t len);

    // ---- 套接字选项 ----

    /// @brief 设置非阻塞模式
    /// @param nonblock true 启用非阻塞，false 启用阻塞
    /// @return 成功返回 true，失败返回 false
    bool setNonBlocking(bool nonblock);

    /// @brief 设置套接字选项（通用接口）
    /// @param level   选项级别
    /// @param optname 选项名
    /// @param optval  选项值指针
    /// @param optlen  选项值长度
    /// @return 成功返回 true，失败返回 false
    bool setSockOpt(int level, int optname, const void* optval, socklen_t optlen);

    // ---- 地址查询 ----

    /// @brief 获取本地地址
    /// @return 本地地址智能指针，失败返回 nullptr
    std::unique_ptr<Address> getLocalAddress() const;

    /// @brief 获取对端地址
    /// @return 对端地址智能指针，失败返回 nullptr
    std::unique_ptr<Address> getPeerAddress() const;

    // ---- 生命周期 ----

    /// @brief 关闭套接字，释放资源
    void close();

    /// @brief 检查套接字是否有效
    /// @return 有效返回 true
    bool isValid() const noexcept;

    /// @brief 获取套接字描述符（供策略和工厂使用）
    /// @return 文件描述符，无效时返回 -1
    int fd() const noexcept;

    /// @brief 获取地址族
    int domain() const noexcept;

    /// @brief 获取套接字类型
    int type() const noexcept;

private:
    /// @brief 私有构造函数，由工厂使用
    /// @param fd       已创建的套接字描述符
    /// @param domain   地址族
    /// @param type     套接字类型
    /// @param protocol 协议
    /// @param strategy 协议策略
    Communicator(int fd, int domain, int type, int protocol,
                 std::unique_ptr<ProtocolStrategy> strategy);

    int                               fd_;
    int                               domain_;
    int                               type_;
    int                               protocol_;
    std::unique_ptr<ProtocolStrategy> strategy_;
};

}  // namespace cdc