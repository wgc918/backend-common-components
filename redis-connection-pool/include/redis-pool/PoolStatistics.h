#pragma once
#include <chrono>
#include <cstdint>

/// Redis 连接池运行统计信息
/// 用于监控连接池的运行状态，支持性能调优和故障排查
struct PoolStatistics
{
    // ---- 连接数量 ----
    int active_connections;  // 当前活跃（已借出）连接数
    int idle_connections;    // 当前空闲（在池中）连接数
    int total_connections;   // 当前连接总数

    // ---- 累计计数 ----
    uint64_t total_borrowed;         // 累计借出次数
    uint64_t total_released;         // 累计归还次数
    uint64_t total_created;          // 累计创建连接数
    uint64_t total_destroyed;        // 累计销毁连接数
    uint64_t total_timeout_errors;   // 累计获取超时错误次数
    uint64_t total_reconnect_count;  // 累计重连次数

    // ---- 等待信息 ----
    int pending_waiters;  // 当前等待获取连接的线程数

    // ---- 耗时统计（毫秒） ----
    int64_t avg_wait_time_ms;  // 平均等待时长
    int64_t max_wait_time_ms;  // 最大等待时长

    // ---- 时间戳 ----
    std::chrono::system_clock::time_point created_at;  // 统计开始时间
    std::chrono::system_clock::time_point updated_at;  // 最后更新时间
};
