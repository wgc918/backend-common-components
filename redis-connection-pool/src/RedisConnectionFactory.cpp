#include "../include/redis-pool/RedisConnectionFactory.h"

namespace rcp
{

sw::redis::Redis* RedisConnectionFactory::create_connection(const RedisConnectionConfig& cfg)
{
    // 配置连接选项
    sw::redis::ConnectionOptions opts;
    opts.host            = cfg.host;
    opts.port            = cfg.port;
    opts.password        = cfg.password;
    opts.db              = cfg.db;
    opts.connect_timeout = std::chrono::milliseconds(cfg.connect_timeout);
    opts.socket_timeout  = std::chrono::milliseconds(cfg.socket_timeout);
    opts.keep_alive      = cfg.keep_alive;

    // 创建连接池选项（单连接场景）
    sw::redis::ConnectionPoolOptions pool_opts;
    pool_opts.size = 1;  // 每个 Redis 对象内部连接池大小为 1

    return new sw::redis::Redis(opts, pool_opts);
}

}  // namespace rcp
