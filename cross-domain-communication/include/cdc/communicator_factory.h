//-----------------------------------------------------------------------------
// 版权所有 (C) 2026 backend-common-components 项目
//
// 文件: communicator_factory.h
// 作者: wgc
// 创建日期: 2026年8月
//
// 描述:
//     通信器工厂模块 — 简化对象创建，根据用户指定模式返回合适的 Communicator 实例。
//     用户无需关心策略选择和地址构造细节。
//
// 功能特性:
//     - CommunicatorConfig 配置结构体：domain/type/protocol
//     - 便捷工厂方法：tcpIpv4 / udpIpv4 / tcpUnix / udpUnix
//     - CommunicatorFactory::create() 自动创建 socket 并选择策略
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

#include "communicator.h"

namespace cdc
{

/// @brief 通信器配置结构体
/// @details 用于指定创建通信器所需的地址族、套接字类型和协议。
struct CommunicatorConfig
{
    int domain;    ///< 地址族：AF_UNIX 或 AF_INET
    int type;      ///< 套接字类型：SOCK_STREAM 或 SOCK_DGRAM
    int protocol;  ///< 协议：0 表示默认协议

    /// @brief 创建 TCP over IPv4 的默认配置
    static CommunicatorConfig tcpIpv4();

    /// @brief 创建 UDP over IPv4 的默认配置
    static CommunicatorConfig udpIpv4();

    /// @brief 创建 TCP over Unix Domain 的默认配置
    static CommunicatorConfig tcpUnix();

    /// @brief 创建 UDP over Unix Domain 的默认配置
    static CommunicatorConfig udpUnix();
};

/// @brief 通信器工厂
/// @details 根据配置创建合适的 Communicator 实例，内部自动选择合适的协议策略。
///          不可实例化。
class CommunicatorFactory
{
public:
    /// @brief 根据配置创建通信器
    /// @param config 通信器配置
    /// @return 创建好的 Communicator 实例，创建失败时 isValid() 返回 false
    static Communicator create(const CommunicatorConfig& config);

private:
    CommunicatorFactory()  = delete;
    ~CommunicatorFactory() = delete;
};

}  // namespace cdc