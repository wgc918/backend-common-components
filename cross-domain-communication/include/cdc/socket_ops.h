//-----------------------------------------------------------------------------
// 版权所有 (C) 2026 backend-common-components 项目
//
// 文件: socket_ops.h
// 作者: wgc
// 创建日期: 2026年8月
//
// 描述:
//     Socket 系统调用封装层，将底层系统调用集中管理，
//     使 Communicator 不直接依赖系统调用，提高可测试性。
//
// 功能特性:
//     - 全静态方法，不可实例化
//     - 封装所有 POSIX socket 系统调用
//     - 所有方法 noexcept，直接透传系统调用返回值
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

#include <sys/socket.h>
#include <sys/types.h>

#include <cstddef>

namespace cdc
{

/// @brief Socket 系统调用封装层
/// @details 所有方法均为静态方法，将系统调用封装为统一接口，
///          使 Communicator 不直接依赖系统调用，提高可测试性。
///          构造函数和析构函数被删除，不可实例化。
class SocketOps
{
public:
    SocketOps()  = delete;
    ~SocketOps() = delete;

    /// @brief 创建套接字
    /// @param domain   地址族 (AF_UNIX / AF_INET)
    /// @param type     套接字类型 (SOCK_STREAM / SOCK_DGRAM)
    /// @param protocol 协议 (0 表示默认)
    /// @return 成功返回文件描述符，失败返回 -1 并设置 errno
    static int socket(int domain, int type, int protocol) noexcept;

    /// @brief 绑定地址
    /// @param fd   套接字文件描述符
    /// @param addr sockaddr 指针
    /// @param len  sockaddr 长度
    /// @return 成功返回 0，失败返回 -1 并设置 errno
    static int bind(int fd, const sockaddr* addr, socklen_t len) noexcept;

    /// @brief 建立连接
    /// @param fd   套接字文件描述符
    /// @param addr 目标 sockaddr 指针
    /// @param len  sockaddr 长度
    /// @return 成功返回 0，失败返回 -1 并设置 errno
    static int connect(int fd, const sockaddr* addr, socklen_t len) noexcept;

    /// @brief 开始监听
    /// @param fd      套接字文件描述符
    /// @param backlog 未完成连接队列的最大长度
    /// @return 成功返回 0，失败返回 -1 并设置 errno
    static int listen(int fd, int backlog) noexcept;

    /// @brief 接受连接
    /// @param fd       监听套接字文件描述符
    /// @param addr     输出参数，对端地址
    /// @param addr_len 输入输出参数，addr 缓冲区大小/实际地址长度
    /// @return 成功返回新连接的文件描述符，失败返回 -1 并设置 errno
    static int accept(int fd, sockaddr* addr, socklen_t* addr_len) noexcept;

    /// @brief 发送数据 (TCP)
    /// @param fd    套接字文件描述符
    /// @param data  数据缓冲区指针
    /// @param len   数据长度
    /// @param flags 发送标志 (默认 0)
    /// @return 成功返回实际发送字节数，失败返回 -1 并设置 errno
    static ssize_t send(int fd, const void* data, size_t len, int flags = 0) noexcept;

    /// @brief 接收数据 (TCP)
    /// @param fd    套接字文件描述符
    /// @param buf   接收缓冲区指针
    /// @param len   缓冲区大小
    /// @param flags 接收标志 (默认 0)
    /// @return 成功返回实际接收字节数（0 表示对端关闭），失败返回 -1 并设置 errno
    static ssize_t recv(int fd, void* buf, size_t len, int flags = 0) noexcept;

    /// @brief 向指定地址发送数据 (UDP)
    /// @param fd       套接字文件描述符
    /// @param data     数据缓冲区指针
    /// @param len      数据长度
    /// @param flags    发送标志
    /// @param addr     目标 sockaddr 指针
    /// @param addr_len sockaddr 长度
    /// @return 成功返回实际发送字节数，失败返回 -1 并设置 errno
    static ssize_t sendto(int fd, const void* data, size_t len, int flags,
                          const sockaddr* addr, socklen_t addr_len) noexcept;

    /// @brief 从任意地址接收数据 (UDP)
    /// @param fd       套接字文件描述符
    /// @param buf      接收缓冲区指针
    /// @param len      缓冲区大小
    /// @param flags    接收标志
    /// @param addr     输出参数，来源地址
    /// @param addr_len 输入输出参数，addr 缓冲区大小/实际来源地址长度
    /// @return 成功返回实际接收字节数，失败返回 -1 并设置 errno
    static ssize_t recvfrom(int fd, void* buf, size_t len, int flags,
                            sockaddr* addr, socklen_t* addr_len) noexcept;

    /// @brief 获取本地地址
    /// @param fd       套接字文件描述符
    /// @param addr     输出参数，本地地址
    /// @param addr_len 输入输出参数
    /// @return 成功返回 0，失败返回 -1 并设置 errno
    static int getsockname(int fd, sockaddr* addr, socklen_t* addr_len) noexcept;

    /// @brief 获取对端地址
    /// @param fd       套接字文件描述符
    /// @param addr     输出参数，对端地址
    /// @param addr_len 输入输出参数
    /// @return 成功返回 0，失败返回 -1 并设置 errno
    static int getpeername(int fd, sockaddr* addr, socklen_t* addr_len) noexcept;

    /// @brief 设置套接字选项
    /// @param fd      套接字文件描述符
    /// @param level   选项级别 (SOL_SOCKET 等)
    /// @param optname 选项名
    /// @param optval  选项值指针
    /// @param optlen  选项值长度
    /// @return 成功返回 0，失败返回 -1 并设置 errno
    static int setsockopt(int fd, int level, int optname,
                          const void* optval, socklen_t optlen) noexcept;

    /// @brief 关闭套接字
    /// @param fd 套接字文件描述符
    /// @return 成功返回 0，失败返回 -1 并设置 errno
    static int close(int fd) noexcept;
};

}  // namespace cdc