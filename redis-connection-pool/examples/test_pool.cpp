#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "../include/redis-pool/RedisConnGuard.h"
#include "../include/redis-pool/RedisConnectionPool.h"

using namespace std;

mutex g_mutx;

/// 基本操作示例：使用连接池进行 SET/GET 操作
void basic_operations()
{
    cout << "=== 1. 基本 SET/GET 操作 ===" << endl;

    auto guard = RedisConnectionPool::instance().get_connection();
    if (!guard)
    {
        cerr << "Failed to get connection" << endl;
        return;
    }

    // 使用 operator-> 直接调用 Redis 命令
    guard->flushdb();
    cout << "flushdb done" << endl;

    guard->set("pool_key", "Hello from connection pool!");
    cout << "SET pool_key done" << endl;

    auto val = guard->get("pool_key");
    if (val)
    {
        cout << "GET pool_key: " << *val << endl;
    }
    else
    {
        cout << "GET pool_key: (nil)" << endl;
    }

    cout << endl;
}

/// 多线程并发示例：10 个线程同时获取连接并执行操作
void multi_thread_test()
{
    cout << "=== 2. 多线程并发测试（10 个线程）===" << endl;

    vector<thread> threads;
    for (int i = 0; i < 10; i++)
    {
        threads.emplace_back(
            [i]()
            {
                auto guard = RedisConnectionPool::instance().get_connection();
                if (!guard)
                {
                    lock_guard<mutex> lock(g_mutx);
                    cerr << "Thread " << i << ": Failed to get connection" << endl;
                    return;
                }

                string key = "thread_key_" + to_string(i);
                guard->set(key, to_string(i * 100));

                auto val = guard->get(key);
                if (val)
                {
                    lock_guard<mutex> lock(g_mutx);
                    cout << "Thread " << i << ": " << key << " = " << *val << endl;
                }

                // guard 析构时自动归还连接
            });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    cout << "All threads completed." << endl << endl;
}

/// 批量操作示例：高效执行多个 Redis 命令
void batch_operations()
{
    cout << "=== 3. 批量操作示例 ===" << endl;

    auto guard = RedisConnectionPool::instance().get_connection();
    if (!guard)
    {
        cerr << "Failed to get connection" << endl;
        return;
    }

    // 批量 SET
    for (int i = 0; i < 5; i++)
    {
        guard->set("batch_key_" + to_string(i), "batch_value_" + to_string(i));
    }
    cout << "Batch SET 5 keys done" << endl;

    // 批量 GET
    for (int i = 0; i < 5; i++)
    {
        auto val = guard->get("batch_key_" + to_string(i));
        if (val)
        {
            cout << "  batch_key_" << i << " = " << *val << endl;
        }
    }

    // 列表操作
    guard->lpush("mylist", {"item1", "item2", "item3"});
    auto len = guard->llen("mylist");
    cout << "List 'mylist' length: " << len << endl;

    // 哈希操作
    guard->hset("myhash", "field1", "value1");
    guard->hset("myhash", "field2", "value2");
    auto hval = guard->hget("myhash", "field1");
    if (hval)
    {
        cout << "Hash myhash.field1 = " << *hval << endl;
    }

    cout << endl;
}

/// 连接池统计信息示例
void print_statistics()
{
    cout << "=== 4. 连接池统计信息 ===" << endl;

    PoolStatistics stats = RedisConnectionPool::instance().get_statistics();

    cout << "--- 连接数量 ---" << endl;
    cout << "  活跃连接:  " << stats.active_connections << endl;
    cout << "  空闲连接:  " << stats.idle_connections << endl;
    cout << "  总连接数:  " << stats.total_connections << endl;

    cout << "--- 累计计数 ---" << endl;
    cout << "  累计借出:  " << stats.total_borrowed << endl;
    cout << "  累计归还:  " << stats.total_released << endl;
    cout << "  累计创建:  " << stats.total_created << endl;
    cout << "  累计销毁:  " << stats.total_destroyed << endl;
    cout << "  超时错误:  " << stats.total_timeout_errors << endl;
    cout << "  重连次数:  " << stats.total_reconnect_count << endl;

    cout << "--- 等待与耗时 ---" << endl;
    cout << "  等待线程:  " << stats.pending_waiters << endl;
    cout << "  平均等待:  " << stats.avg_wait_time_ms << " ms" << endl;
    cout << "  最大等待:  " << stats.max_wait_time_ms << " ms" << endl;

    cout << endl;
}

/// 连接池配置信息示例
void print_config()
{
    cout << "=== 5. 连接池配置 ===" << endl;

    auto& pool = RedisConnectionPool::instance();
    cout << "  最小连接数:  " << pool.get_min_size() << endl;
    cout << "  最大连接数:  " << pool.get_max_size() << endl;
    cout << "  最大空闲时间: " << pool.get_max_idle_time() << " 秒" << endl;
    cout << "  获取超时:    " << pool.get_timeout() << " 毫秒" << endl;

    cout << endl;
}

/// 移动语义示例：在函数间传递连接守卫
void process_with_guard(RedisConnGuard&& guard)
{
    if (!guard)
    {
        cerr << "No connection available" << endl;
        return;
    }

    guard->set("moved_guard_key", "Transferred via move semantics");
    auto val = guard->get("moved_guard_key");
    if (val)
    {
        cout << "  Moved guard value: " << *val << endl;
    }
}

void move_semantics_test()
{
    cout << "=== 6. 移动语义测试 ===" << endl;

    auto guard = RedisConnectionPool::instance().get_connection();
    if (!guard)
    {
        cerr << "Failed to get connection" << endl;
        return;
    }

    // 使用 std::move 将连接守卫移动到函数中
    process_with_guard(std::move(guard));

    // guard 已被移动，不再持有连接
    if (!guard)
    {
        cout << "  Original guard is now empty (moved)" << endl;
    }

    cout << endl;
}

int main()
{
    cout << "========================================" << endl;
    cout << "  Redis 连接池框架 - 综合示例" << endl;
    cout << "========================================" << endl << endl;

    // 配置连接参数
    RedisConnectionConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = 6379;
    // cfg.password = "your_password";  // 如有密码请取消注释
    cfg.db              = 0;
    cfg.connect_timeout = 1000;
    cfg.socket_timeout  = 1000;
    cfg.keep_alive      = true;

    // 初始化连接池（最小 4 个连接，最大 20 个连接，空闲 180 秒回收，获取超时 10 秒，每次扩容 4 个）
    auto& pool = RedisConnectionPool::instance();
    pool.init(cfg, 4, 20, 180, 10000, 4);

    cout << "Redis connection pool initialized successfully!" << endl << endl;

    try
    {
        // 打印配置
        print_config();

        // 打印统计信息
        print_statistics();

        // 基本操作
        basic_operations();

        // 打印统计信息
        print_statistics();

        // 批量操作
        batch_operations();

        // 多线程并发
        multi_thread_test();

        // 移动语义
        move_semantics_test();

        // 打印统计信息
        print_statistics();
    }
    catch (const exception& e)
    {
        cerr << "Error: " << e.what() << endl;
    }

    // 优雅关闭连接池
    cout << "Shutting down connection pool..." << endl;
    pool.shutdown();  // 可能持续时间会比较长，因为后台管理线程设置的频率是30s一次，关掉之前需要先销毁它
    cout << "Connection pool shut down successfully." << endl;

    return 0;
}
