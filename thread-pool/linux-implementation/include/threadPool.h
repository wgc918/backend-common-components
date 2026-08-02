/*
 * =============================================================================
 * Project:       [redis-like]
 * Module:        thread_pool (Linux/pthread implementation)
 * Author:        GuochengWu
 * Email:         [3524515056@qq.com]
 *
 * Copyright (c) 2026 GuochengWu. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Description:   Linux/pthread版本线程池 —— 基于Linux原生pthread接口实现的线程池。
 *                核心职责包括：
 *                1. 基于生产者-消费者模型设计，使用 pthread_mutex_t / pthread_cond_t 等
 *                   POSIX 同步原语，保障任务提交与执行的线程安全；
 *                2. 实现「管理者线程+工作线程组」的双层架构，管理者线程按指定频率监控线程池状态，
 *                   动态扩缩容工作线程数（基于最小/最大线程数、忙闲状态），兼顾资源利用率与响应性能；
 *                3. 提供标准化的任务提交接口，支持任务队列容量限制、线程退出/销毁的安全管控，
 *                   禁用拷贝/移动语义保障线程池实例的唯一性与稳定性；
 *                4. 为上层模块提供高可用的异步任务执行能力，支撑并行处理，提升系统并发处理能力。
 *
 * Version:       1.0.0
 * Last Modified: 2026-08-03
 * =============================================================================
 */

#pragma once

#include <vector>
#include <queue>
#include <pthread.h>
#include <chrono>
#include <atomic>

#include "task.h"

class ThreadPool
{
public:
    ThreadPool(int min, int max, int op_num, size_t cap);
    ~ThreadPool();

    // 禁用拷贝与移动
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    void submit(Task *task);
    void set_check_freq(int check_freq);
    int  get_live_threads();
    int  get_busy_threads();

private:
    Task *getTask();

    // 管理者线程函数
    static void *manager(void *arg);
    // 工作线程函数
    static void *work(void *arg);
    // 线程退出清理
    static void thread_exit_(ThreadPool *pool);

private:
    std::queue<Task *> m_taskQ;
    size_t             m_capacity;                    // 任务队列容量
    pthread_cond_t     m_full;                        // 队列满时阻塞生产者
    pthread_cond_t     m_empty;                       // 队列空时阻塞消费者
    pthread_mutex_t    m_queue_mutex;                  // 任务队列互斥锁

    std::vector<pthread_t> m_thread_work_group;        // 工作线程组（预分配 max 个槽位）
    pthread_t              m_manager_t;                 // 管理者线程
    pthread_mutex_t        m_pool_mutex;                // 线程池状态互斥锁

    int m_min_threads;
    int m_max_threads;
    int m_live_threads;
    int m_busy_threads;
    int m_exit_threads;       // 待销毁线程数
    const int m_op_num;       // 每次扩缩容最大操作数
    std::atomic<bool> m_is_shutdown;
    int  m_check_freq;        // 管理者线程巡检间隔（秒）
};