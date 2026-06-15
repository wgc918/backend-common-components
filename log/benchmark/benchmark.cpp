/*
 * =============================================================================
 * Module:        logger 基准测试
 * Purpose:       对比 std::stringstream vs snprintf 的性能差异
 * =============================================================================
 *
 * 测试项目:
 *   1. 纯格式化性能：stringstream vs snprintf 栈缓冲区
 *   2. 日志写入吞吐量：不同日志级别下的每秒写入条数
 *   3. 内存占用对比：堆分配次数与内存峰值
 *   4. 多线程并发写入性能
 *
 * 编译方法:
 *   g++ -std=c++17 -O2 -DNDEBUG -pthread \
 *       benchmark.cpp ../src/log.cpp \
 *       -I../include -o benchmark
 *
 * 运行:
 *   ./benchmark [iterations] [threads]
 *   ./benchmark 100000 4
 * =============================================================================
 */

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <ratio>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "../include/log.h"

// =============================================================================
// 测试工具
// =============================================================================

class Timer
{
public:
    using Clock     = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;

    void start()
    {
        m_start = Clock::now();
    }

    void stop()
    {
        m_end = Clock::now();
    }

    double elapsed_ms() const
    {
        return std::chrono::duration<double, std::milli>(m_end - m_start).count();
    }

    double elapsed_us() const
    {
        return std::chrono::duration<double, std::micro>(m_end - m_start).count();
    }

private:
    TimePoint m_start;
    TimePoint m_end;
};

// =============================================================================
// 测试 1: stringstream 格式化（模拟原始实现）
// =============================================================================

volatile char g_dummy_char = 0;  // 防止编译器优化掉

void bench_stringstream_format(int iterations)
{
    // 模拟原始 Logger::log() 中的格式化逻辑
    auto now    = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    Timer timer;
    timer.start();

    for (int i = 0; i < iterations; ++i)
    {
        std::stringstream ss;

        // 时间戳
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        std::string tist = ss.str();

        ss.str("");
        ss.clear();

        // 日志消息
        ss << "[" << tist << "] [INFO] [test.cpp:" << 42 << "] "
           << "key=" << i << ", value=" << (i * 3.14) << ", msg=hello_world_test\n";
        std::string result = ss.str();

        g_dummy_char = result[0];  // 防止优化
    }

    timer.stop();
    std::cout << "[stringstream format] " << iterations << " iterations: " << timer.elapsed_ms()
              << " ms (" << timer.elapsed_us() / iterations << " us/op)" << std::endl;
}

// =============================================================================
// 测试 2: snprintf 栈缓冲区格式化（优化实现）
// =============================================================================

void bench_snprintf_format(int iterations)
{
    auto now    = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    Timer timer;
    timer.start();

    for (int i = 0; i < iterations; ++i)
    {
        char buf[4096];

        std::tm local_time;
        localtime_r(&time_t, &local_time);

        int len = snprintf(buf, sizeof(buf),
                           "%04d-%02d-%02d %02d:%02d:%02d.%03lld [INFO] [test.cpp:%d] "
                           "key=%d, value=%.2f, msg=hello_world_test\n",
                           local_time.tm_year + 1900, local_time.tm_mon + 1, local_time.tm_mday,
                           local_time.tm_hour, local_time.tm_min, local_time.tm_sec,
                           static_cast<long long>(ms.count()), 42, i, i * 3.14);

        g_dummy_char = buf[0];  // 防止优化
        (void)len;
    }

    timer.stop();
    std::cout << "[snprintf   format] " << iterations << " iterations: " << timer.elapsed_ms()
              << " ms (" << timer.elapsed_us() / iterations << " us/op)" << std::endl;
}

// =============================================================================
// 测试 3: 完整日志写入吞吐量（新版 Logger）
// =============================================================================

void bench_logger_throughput(int iterations)
{
    // 初始化 Logger（仅文件输出，避免控制台开销）
    LogConfig config;
    config.file_name = "/tmp/bench_test.log";
    config.console   = false;
    config.max_lines = 0;  // 不轮转
    Logger::instance().init(config);

    Timer timer;
    timer.start();

    for (int i = 0; i < iterations; ++i)
    {
        LOGI("key=", i, ", value=", i * 3.14, ", msg=hello_world_test");
    }

    Logger::instance().shutdown();
    timer.stop();

    double ops_per_sec = iterations / (timer.elapsed_ms() / 1000.0);

    std::cout << "[Logger  throughput] " << iterations << " iterations: " << timer.elapsed_ms()
              << " ms (" << static_cast<long long>(ops_per_sec) << " ops/sec)" << std::endl;

    // 清理
    std::remove("/tmp/bench_test.log");
}

// =============================================================================
// 测试 4: 批量日志写入（对比 flush 频率影响）
// =============================================================================

void bench_logger_batched(int iterations)
{
    LogConfig config;
    config.file_name = "/tmp/bench_batched.log";
    config.console   = false;
    config.max_lines = 0;
    Logger::instance().init(config);

    Timer timer;
    timer.start();

    for (int i = 0; i < iterations; ++i)
    {
        // 全部使用 DEBUG 级别，触发批量缓冲（仅半满或 ERROR 时刷新）
        LOGD("key=", i, ", value=", i * 3.14, ", msg=hello_world_test");
    }

    Logger::instance().shutdown();
    timer.stop();

    double ops_per_sec = iterations / (timer.elapsed_ms() / 1000.0);

    std::cout << "[Logger  batched  ] " << iterations << " iterations: " << timer.elapsed_ms()
              << " ms (" << static_cast<long long>(ops_per_sec) << " ops/sec)" << std::endl;

    std::remove("/tmp/bench_batched.log");
}

// =============================================================================
// 测试 5: 多线程并发写入
// =============================================================================

void bench_logger_multithread(int iterations, int num_threads)
{
    LogConfig config;
    config.file_name = "/tmp/bench_mt.log";
    config.console   = false;
    config.max_lines = 0;
    Logger::instance().init(config);

    std::atomic<bool>        start_flag{false};
    std::vector<std::thread> threads;

    Timer timer;

    for (int t = 0; t < num_threads; ++t)
    {
        threads.emplace_back(
            [&, t, iterations]()
            {
                while (!start_flag.load(std::memory_order_acquire))
                    ;

                for (int i = 0; i < iterations; ++i)
                {
                    LOGI("thread=", t, ", key=", i, ", value=", i * 3.14, ", msg=hello_world_test");
                }
            });
    }

    // 同步启动
    timer.start();
    start_flag.store(true, std::memory_order_release);

    for (auto& th : threads)
        th.join();

    Logger::instance().shutdown();
    timer.stop();

    long long total_ops   = static_cast<long long>(iterations) * num_threads;
    double    ops_per_sec = total_ops / (timer.elapsed_ms() / 1000.0);

    std::cout << "[Logger  MT " << num_threads << "T ] " << total_ops
              << " total ops: " << timer.elapsed_ms() << " ms ("
              << static_cast<long long>(ops_per_sec) << " ops/sec)" << std::endl;

    std::remove("/tmp/bench_mt.log");
}

// =============================================================================
// 测试 6: 内存分配次数对比（通过 override operator new 计数）
// =============================================================================

static std::atomic<long long> g_alloc_count{0};
static std::atomic<long long> g_alloc_bytes{0};

struct AllocTracker
{
    AllocTracker()
    {
        g_alloc_count = 0;
        g_alloc_bytes = 0;
    }

    ~AllocTracker() = default;
};

void* operator new(size_t size)
{
    g_alloc_count.fetch_add(1, std::memory_order_relaxed);
    g_alloc_bytes.fetch_add(static_cast<long long>(size), std::memory_order_relaxed);
    return std::malloc(size);
}

void operator delete(void* ptr) noexcept
{
    std::free(ptr);
}

void operator delete(void* ptr, size_t) noexcept
{
    std::free(ptr);
}

void bench_memory_stringstream(int iterations)
{
    AllocTracker tracker;

    long long before_count = g_alloc_count.load();
    long long before_bytes = g_alloc_bytes.load();

    bench_stringstream_format(iterations);

    long long after_count = g_alloc_count.load();
    long long after_bytes = g_alloc_bytes.load();

    std::cout << "[Memory stringstream] " << iterations << " ops: " << (after_count - before_count)
              << " allocations, " << (after_bytes - before_bytes) / 1024 << " KB total"
              << std::endl;
}

void bench_memory_snprintf(int iterations)
{
    long long before_count = g_alloc_count.load();
    long long before_bytes = g_alloc_bytes.load();

    bench_snprintf_format(iterations);

    long long after_count = g_alloc_count.load();
    long long after_bytes = g_alloc_bytes.load();

    std::cout << "[Memory snprintf    ] " << iterations << " ops: " << (after_count - before_count)
              << " allocations, " << (after_bytes - before_bytes) / 1024 << " KB total"
              << std::endl;
}

// =============================================================================
// 主函数
// =============================================================================

int main(int argc, char* argv[])
{
    int iterations  = (argc > 1) ? std::atoi(argv[1]) : 100000;
    int num_threads = (argc > 2) ? std::atoi(argv[2]) : 4;

    std::cout << "============================================================" << std::endl;
    std::cout << "  Logger 性能基准测试" << std::endl;
    std::cout << "  Iterations: " << iterations << "  |  Threads: " << num_threads << std::endl;
    std::cout << "============================================================" << std::endl;
    std::cout << std::endl;

    // ---- 测试 1: 纯格式化对比 ----
    std::cout << "--- 1. 纯格式化性能对比 ---" << std::endl;
    bench_stringstream_format(iterations);
    bench_snprintf_format(iterations);
    double ss_ms = 0, snp_ms = 0;
    {
        // 快速计算大致比例
        Timer t;
        t.start();
        bench_stringstream_format(iterations / 10);
        t.stop();
        ss_ms = t.elapsed_ms() * 10;
    }
    {
        Timer t;
        t.start();
        bench_snprintf_format(iterations / 10);
        t.stop();
        snp_ms = t.elapsed_ms() * 10;
    }
    if (snp_ms > 0)
        std::cout << "  => snprintf 比 stringstream 快约 " << static_cast<int>(ss_ms / snp_ms)
                  << "x" << std::endl;
    std::cout << std::endl;

    // ---- 测试 2: 内存分配对比 ----
    std::cout << "--- 2. 内存分配对比 ---" << std::endl;
    bench_memory_stringstream(iterations);
    bench_memory_snprintf(iterations);
    std::cout << std::endl;

    // ---- 测试 3: 日志写入吞吐量 ----
    std::cout << "--- 3. 日志写入吞吐量 ---" << std::endl;
    bench_logger_throughput(iterations);
    bench_logger_batched(iterations);
    std::cout << std::endl;

    // ---- 测试 4: 多线程并发 ----
    std::cout << "--- 4. 多线程并发写入 ---" << std::endl;
    bench_logger_multithread(iterations / num_threads, num_threads);
    bench_logger_multithread(iterations / num_threads, 1);  // 单线程对照
    std::cout << std::endl;

    std::cout << "============================================================" << std::endl;
    std::cout << "  测试完成" << std::endl;
    std::cout << "============================================================" << std::endl;

    return 0;
}