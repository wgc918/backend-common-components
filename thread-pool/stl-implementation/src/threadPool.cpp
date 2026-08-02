#include "../include/threadPool.h"

#include <algorithm>

#include "../include/simple_logger.h"

ThreadPool::ThreadPool(int min, int max, int op_num, size_t cap) : m_op_num(op_num)
{
    // 任务队列相关
    m_capacity = cap;

    // 线程池相关
    m_thread_work_group.resize(max);  // 预分配 max 个默认构造的 std::thread（非 joinable）
    m_min_threads  = min;
    m_max_threads  = max;
    m_live_threads = min;
    m_busy_threads = 0;
    m_exit_threads = 0;
    m_is_shutdown  = false;
    m_check_freq   = 1;

    // 启动管理者线程
    m_manager_thread = std::thread(manager, this);

    // 创建最小数量的工作线程
    for (int i = 0; i < min; i++)
    {
        m_thread_work_group[i] = std::thread(work, this);
    }

    LOG_DEBUG("Success to init threadPool (STL), min_threads: " + std::to_string(min) +
              ", max_threads: " + std::to_string(max));
}

ThreadPool::~ThreadPool()
{
    m_is_shutdown = true;

    // 唤醒所有等待任务的线程
    {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
    }
    m_empty.notify_all();

    // 等待管理者线程退出
    if (m_manager_thread.joinable())
    {
        m_manager_thread.join();
    }

    LOG_DEBUG("The thread of manager exit...");

    // 等待所有工作线程退出
    while (true)
    {
        {
            std::lock_guard<std::mutex> lock(m_pool_mutex);
            if (m_live_threads == 0)
                break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    LOG_DEBUG("The threadPool exit...");
}

void ThreadPool::submit(Task* task)
{
    if (task == nullptr)
    {
        LOG_ERROR("Failed to submit: task is nullptr.");
        return;
    }
    if (m_is_shutdown)
    {
        LOG_ERROR("Failed to submit: threadPool is shutdown.");
        delete task;
        return;
    }

    std::unique_lock<std::mutex> lock(m_queue_mutex);

    while (m_taskQ.size() >= m_capacity && !m_is_shutdown)
    {
        // 阻塞生产者线程
        m_full.wait(lock);
    }

    m_taskQ.push(task);
    m_empty.notify_one();
}

void ThreadPool::set_check_freq(int check_freq)
{
    std::lock_guard<std::mutex> lock(m_pool_mutex);
    m_check_freq = check_freq;
}

int ThreadPool::get_live_threads()
{
    std::lock_guard<std::mutex> lock(m_pool_mutex);
    return m_live_threads;
}

int ThreadPool::get_busy_threads()
{
    std::lock_guard<std::mutex> lock(m_pool_mutex);
    return m_busy_threads;
}

Task* ThreadPool::getTask()
{
    Task* ret = nullptr;
    if (!m_taskQ.empty())
    {
        ret = m_taskQ.front();
        m_taskQ.pop();
    }
    return ret;
}

void ThreadPool::manager(ThreadPool* pool)
{
    while (!pool->m_is_shutdown)
    {
        std::this_thread::sleep_for(std::chrono::seconds(pool->m_check_freq));

        // 获取队列大小
        int qsize = 0;
        {
            std::lock_guard<std::mutex> lock(pool->m_queue_mutex);
            qsize = static_cast<int>(pool->m_taskQ.size());
        }

        // 获取线程池状态
        int live_num = 0;
        int busy_num = 0;
        {
            std::lock_guard<std::mutex> lock(pool->m_pool_mutex);
            live_num = pool->m_live_threads;
            busy_num = pool->m_busy_threads;
        }

        // ===== 添加线程 =====
        // 条件：任务数 > 存活线程数 && 存活线程数 < 最大线程数
        if (qsize > live_num && live_num < pool->m_max_threads)
        {
            std::lock_guard<std::mutex> lock(pool->m_pool_mutex);
            int                         cnt = 0;
            for (int i = 0; i < pool->m_max_threads && cnt < pool->m_op_num &&
                            pool->m_live_threads < pool->m_max_threads;
                 i++)
            {
                if (!pool->m_thread_work_group[i].joinable())
                {
                    pool->m_thread_work_group[i] = std::thread(work, pool);
                    pool->m_live_threads++;
                    cnt++;
                    LOG_DEBUG("Add a new thread: cur_live_threads = " +
                              std::to_string(pool->m_live_threads));
                }
            }
        }

        // ===== 销毁线程 =====
        // 条件：忙线程数*2 < 存活线程数 && 存活线程数 > 最小线程数
        if (((busy_num * 2 < live_num) || (qsize == 0 && busy_num == 0)) &&
            live_num > pool->m_min_threads)
        {
            int can_exit = 0;
            {
                std::lock_guard<std::mutex> lock(pool->m_pool_mutex);
                can_exit             = std::min(pool->m_op_num, live_num - pool->m_min_threads);
                pool->m_exit_threads = can_exit;
            }

            if (can_exit > 0)
            {
                LOG_DEBUG("Prepare to destroy " + std::to_string(can_exit) +
                          " threads: cur_live_threads = " + std::to_string(live_num));
                // 唤醒对应数量的线程让其退出
                for (int i = 0; i < can_exit; i++)
                {
                    pool->m_empty.notify_one();
                }
                // 短暂等待，让线程有时间处理退出
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        }
    }
}

void ThreadPool::work(ThreadPool* pool)
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(pool->m_queue_mutex);

        while (pool->m_taskQ.empty() && !pool->m_is_shutdown)
        {
            pool->m_empty.wait(lock);

            // 检查是否需要退出（先检查，再释放 pool_lock 后调用 thread_exit_）
            bool should_exit = false;
            {
                std::lock_guard<std::mutex> pool_lock(pool->m_pool_mutex);
                if (pool->m_exit_threads > 0)
                {
                    pool->m_exit_threads--;
                    pool->m_live_threads--;
                    should_exit = true;
                }
            }
            if (should_exit)
            {
                lock.unlock();
                thread_exit_(pool);
                return;
            }
        }

        // 线程池关闭
        if (pool->m_is_shutdown)
        {
            {
                std::lock_guard<std::mutex> pool_lock(pool->m_pool_mutex);
                pool->m_live_threads--;
            }
            lock.unlock();
            thread_exit_(pool);
            return;
        }

        // 获取任务
        Task* task = pool->getTask();
        pool->m_full.notify_one();
        lock.unlock();

        // 执行任务
        {
            std::lock_guard<std::mutex> pool_lock(pool->m_pool_mutex);
            pool->m_busy_threads++;
        }

        task->run();
        delete task;

        {
            std::lock_guard<std::mutex> pool_lock(pool->m_pool_mutex);
            pool->m_busy_threads--;
        }
    }
}

void ThreadPool::thread_exit_(ThreadPool* pool)
{
    std::thread::id tid = std::this_thread::get_id();

    std::lock_guard<std::mutex> lock(pool->m_pool_mutex);
    for (auto& t : pool->m_thread_work_group)
    {
        if (t.joinable() && t.get_id() == tid)
        {
            t.detach();         // 分离线程，允许其独立结束
            t = std::thread();  // 重置槽位为默认（非 joinable）
            LOG_DEBUG("A thread is exiting...");
            break;
        }
    }
}