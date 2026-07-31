//-----------------------------------------------------------------------------
// 版权所有 (C) 2026 backend-common-components 项目
//
// 文件: example_echo.cpp
// 作者: wgc
// 创建日期: 2026年8月
//
// 描述:
//     TCP 回显服务器示例 — 演示跨域通信组件的基本用法。
//     服务器端：bind → listen → accept → recv → send → close
//     客户端：connect → send → recv → close
//
// 许可证:
//     MIT License
//-----------------------------------------------------------------------------

#include "cdc/cdc.h"

#include <cstdio>
#include <cstring>
#include <iostream>
#include <thread>
#include <unistd.h>

static const char* SOCK_PATH = "/tmp/cdc_echo_example.sock";

/// @brief TCP 回显服务器
void echo_server()
{
    // 创建 TCP over Unix Domain Socket 通信器
    auto server = cdc::CommunicatorFactory::create(cdc::CommunicatorConfig::tcpUnix());

    if (!server.isValid())
    {
        std::cerr << "[Server] Failed to create server communicator" << std::endl;
        return;
    }

    // 绑定地址
    auto addr = cdc::UnixAddress(SOCK_PATH);
    if (!server.bind(addr))
    {
        std::cerr << "[Server] Failed to bind: " << std::strerror(errno) << std::endl;
        return;
    }

    // 监听
    if (!server.listen(1))
    {
        std::cerr << "[Server] Failed to listen: " << std::strerror(errno) << std::endl;
        return;
    }

    std::cout << "[Server] Listening on " << addr.toString() << std::endl;

    // 接受连接
    auto client = server.accept();
    if (!client)
    {
        std::cerr << "[Server] Failed to accept: " << std::strerror(errno) << std::endl;
        return;
    }

    // 获取对端地址
    auto peer = client->getPeerAddress();
    if (peer)
    {
        std::cout << "[Server] Client connected from " << peer->toString() << std::endl;
    }

    // 接收数据
    char buf[1024];
    ssize_t n = client->recv(buf, sizeof(buf) - 1);
    if (n > 0)
    {
        buf[n] = '\0';
        std::cout << "[Server] Received: " << buf << std::endl;

        // 回显
        ssize_t sent = client->send(buf, static_cast<size_t>(n));
        std::cout << "[Server] Echoed " << sent << " bytes" << std::endl;
    }
    else if (n < 0)
    {
        std::cerr << "[Server] recv error: " << std::strerror(errno) << std::endl;
    }

    client->close();
    server.close();
    ::unlink(SOCK_PATH);
}

/// @brief TCP 回显客户端
void echo_client()
{
    ::sleep(1);  // 等待服务器启动

    auto client = cdc::CommunicatorFactory::create(cdc::CommunicatorConfig::tcpUnix());

    if (!client.isValid())
    {
        std::cerr << "[Client] Failed to create client communicator" << std::endl;
        return;
    }

    auto addr = cdc::UnixAddress(SOCK_PATH);
    if (!client.connect(addr))
    {
        std::cerr << "[Client] Failed to connect: " << std::strerror(errno) << std::endl;
        return;
    }

    std::cout << "[Client] Connected to " << addr.toString() << std::endl;

    // 获取本地地址
    auto local = client.getLocalAddress();
    if (local)
    {
        std::cout << "[Client] Local address: " << local->toString() << std::endl;
    }

    const char* msg = "Hello, CDC!";
    ssize_t sent = client.send(msg, std::strlen(msg));
    if (sent < 0)
    {
        std::cerr << "[Client] send error: " << std::strerror(errno) << std::endl;
        client.close();
        return;
    }
    std::cout << "[Client] Sent " << sent << " bytes: " << msg << std::endl;

    char buf[1024];
    ssize_t n = client.recv(buf, sizeof(buf) - 1);
    if (n > 0)
    {
        buf[n] = '\0';
        std::cout << "[Client] Received: " << buf << std::endl;
    }
    else if (n < 0)
    {
        std::cerr << "[Client] recv error: " << std::strerror(errno) << std::endl;
    }

    client.close();
}

int main()
{
    ::unlink(SOCK_PATH);  // 清理残留文件

    std::cout << "=== CDC Echo Example (TCP over Unix Domain Socket) ===" << std::endl;

    std::thread server_thread(echo_server);
    std::thread client_thread(echo_client);

    server_thread.join();
    client_thread.join();

    std::cout << "=== Done ===" << std::endl;
    return 0;
}