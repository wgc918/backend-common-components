#pragma once
#include <string>

#include <sw/redis++/redis++.h>

namespace rcp
{

/// Redis 连接配置参数
struct RedisConnectionConfig
{
    std::string host            = "127.0.0.1";  // Redis 服务器地址
    int         port            = 6379;          // Redis 端口
    std::string password;                        // 密码（默认为空，无密码）
    int         db              = 0;             // 数据库编号
    int         connect_timeout = 1000;          // 连接超时（毫秒）
    int         socket_timeout  = 1000;          // socket 读写超时（毫秒）
    bool        keep_alive      = true;          // 是否启用 TCP keepalive
};

/// Redis 连接工厂
/// 根据配置创建 sw::redis::Redis 连接对象
class RedisConnectionFactory
{
public:
    /// 根据配置创建 Redis 连接
    /// @param cfg 连接配置参数
    /// @return Redis 连接对象（调用方负责释放）
    /// @throws sw::redis::Error 连接创建失败时抛出异常
    sw::redis::Redis* create_connection(const RedisConnectionConfig& cfg);
};

} // namespace rcp
