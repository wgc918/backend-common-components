//-----------------------------------------------------------------------------
// 版权所有 (C) 2026 rabbitMQ-pool 项目
//
// 文件: connection.h
// 作者: wgc
// 创建日期: 2026年7月
// 最后修改: 2026年8月
//
// 描述:
//     封装 AMQP 连接，管理 TCP 连接的生命周期与线程安全。
//
// 功能特性:
//     - ConnConfig 结构体：集中管理连接参数（主机、端口、认证等）
//     - Connection 类：建立、重连、断开 AMQP 连接
//     - 提供内部互斥锁引用，供 Channel 层进行线程安全操作
//
// 许可证:
//     MIT License
//
//     版权所有 (c) 2026 wgc
//
//     特此免费授予获得本软件副本和相关文档文件（以下简称"软件"）的任何人
//     以处理软件的权利，包括但不限于使用、复制、修改、合并、出版、分发、
//     再许可和/或出售软件副本，以及允许软件适用者这样做，须在下列条件下：
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

#include <amqp.h>
#include <amqp_tcp_socket.h>

#include <cstdint>
#include <mutex>
#include <string>

namespace rmq
{

/// @brief AMQP 连接配置
struct ConnConfig
{
    std::string host        = "127.0.0.1";  ///< 服务器地址
    int         port        = 5672;         ///< 服务器端口
    std::string user        = "zsr";        ///< 用户名
    std::string password    = "123456";     ///< 密码
    std::string vhost       = "/";          ///< 虚拟主机
    int         channel_max = 0;            ///< 最大通道数（0 使用服务器默认值）
    int         frame_max   = 0;            ///< 最大帧大小（0 使用服务器默认值）
    int         heartbeat   = 0;            ///< 心跳间隔（0 禁用心跳）
};

/// @brief AMQP 连接封装
/// @details
///     管理到 RabbitMQ 服务器的 TCP 连接生命周期。
///     通过 RAII 自动管理连接资源，支持断线重连。
///     提供互斥锁供 Channel 层在操作时加锁，保证线程安全。
class Connection
{
public:
    // ============================================
    // 构造函数与析构函数
    // ============================================

    Connection();
    Connection(const Connection& other)            = delete;
    Connection& operator=(const Connection& other) = delete;
    Connection(Connection&& other);
    Connection& operator=(Connection&& other);
    ~Connection();

    // ============================================
    // 连接管理
    // ============================================

    /// @brief 使用配置初始化连接
    /// @param cfg 连接配置
    /// @return true 表示初始化成功，false 表示失败
    bool init(const ConnConfig& cfg);

    /// @brief 重新连接
    /// @return true 表示重连成功，false 表示失败
    bool reconnect();

    /// @brief 检查当前是否已连接
    bool is_connected() const;

    /// @brief 获取底层 rabbitmq-c 连接状态
    amqp_connection_state_t connection() const;

    /// @brief 返回内部互斥锁引用，供 Channel 层在操作前加锁
    std::mutex& mutex();

private:
    amqp_connection_state_t m_conn;    ///< rabbitmq-c 连接状态
    bool                    m_init;    ///< 是否已初始化
    ConnConfig              m_config;  ///< 连接配置
    mutable std::mutex      m_mutex;   ///< 线程安全互斥锁
};

}  // namespace rmq