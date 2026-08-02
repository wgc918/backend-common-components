#include <unistd.h>
#include <algorithm>
#include "../include/threadPool.h"
#include "../include/simple_logger.h"

ThreadPool::ThreadPool(int min, int max, int op_num, size_t cap)
    : m_op_num(op_num)
{
    // 任务队列相关
    m_capacity = cap;
    pthread_cond_init(&m_full, nullptr);
    pthread_cond_init(&m_empty, nullptr);
    pthread_mutex_init(&m_queue_mutex, nullptr);

    // 线程池相关
    m_thread_work_group.assign(max, 0);
    pthread_mutex_init(&m_pool_mutex, nullptr);
    m_min_threads  = min;
    m_max_threads  = max;
    m_live_threads = min;
    m_busy_threads = 0;
    m_exit_threads = 0;
    m_is_shutdown  = false;
    m_check_freq   = 1;

    pthread_create(&m_manager_t, nullptr, manager, this);
    for (int i = 0; i < min; i++)
    {
        pthread_create(&m_thread_work_group[i], nullptr, work, this);
    }

    LOG_DEBUG("Success to init threadPool (Linux/pthread), min_threads: " +
              std::to_string(min) + ", max_threads: " + std::to_string(max));
}

ThreadPool::~ThreadPool()
{
    m_is_shutdown = true;

    // 唤醒所有等待任务的线程
    pthread_mutex_lock(&m_queue_mutex);
    pthread_cond_broadcast(&m_empty);
    pthread_mutex_unlock(&m_queue_mutex);

    pthread_join(m_manager_t, nullptr);

    LOG_DEBUG("The thread of manager exit...");

    // 等待所有工作线程退出
    while (true)
    {
        pthread_mutex_lock(&m_pool_mutex);
        int live = m_live_threads;
        pthread_mutex_unlock(&m_pool_mutex);
        if (live == 0)
            break;
        usleep(10000); // 10ms
    }

    pthread_cond_destroy(&m_empty);
    pthread_cond_destroy(&m_full);
    pthread_mutex_destroy(&m_pool_mutex);
    pthread_mutex_destroy(&m_queue_mutex);

    LOG_DEBUG("The threadPool exit...");
}

void ThreadPool::submit(Task *task)
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

    pthread_mutex_lock(&m_queue_mutex);

    while (m_taskQ.size() >= m_capacity && !m_is_shutdown)
    {
        // 阻塞生产者线程
        pthread_cond_wait(&m_full, &m_queue_mutex);
    }

    m_taskQ.push(task);
    pthread_cond_signal(&m_empty);

    pthread_mutex_unlock(&m_queue_mutex);
}

void ThreadPool::set_check_freq(int check_freq)
{
    pthread_mutex_lock(&m_pool_mutex);
    m_check_freq = check_freq;
    pthread_mutex_unlock(&m_pool_mutex);
}

int ThreadPool::get_live_threads()
{
    pthread_mutex_lock(&m_pool_mutex);
    int ret = m_live_threads;
    pthread_mutex_unlock(&m_pool_mutex);
    return ret;
}

int ThreadPool::get_busy_threads()
{
    pthread_mutex_lock(&m_pool_mutex);
    int ret = m_busy_threads;
    pthread_mutex_unlock(&m_pool_mutex);
    return ret;
}

Task *ThreadPool::getTask()
{
    Task *ret = nullptr;
    if (!m_taskQ.empty())
    {
        ret = m_taskQ.front();
        m_taskQ.pop();
    }
    return ret;
}

void *ThreadPool::manager(void *arg)
{
    ThreadPool *pool = static_cast<ThreadPool *>(arg);

    while (!pool->m_is_shutdown)
    {
        sleep(pool->m_check_freq);

        pthread_mutex_lock(&pool->m_queue_mutex);
        int qsize = static_cast<int>(pool->m_taskQ.size());
        pthread_mutex_unlock(&pool->m_queue_mutex);

        pthread_mutex_lock(&pool->m_pool_mutex);
        int live_num = pool->m_live_threads;
        int busy_num = pool->m_busy_threads;
        pthread_mutex_unlock(&pool->m_pool_mutex);

        // ===== 添加线程 =====
        // 条件：任务数 > 存活线程数 && 存活线程数 < 最大线程数
        if (qsize > live_num && live_num < pool->m_max_threads)
        {
            pthread_mutex_lock(&pool->m_pool_mutex);
            int cnt = 0;
            for (int i = 0;
                 i < pool->m_max_threads && cnt < pool->m_op_num && pool->m_live_threads < pool->m_max_threads;
                 i++)
            {
                if (pool->m_thread_work_group[i] == 0)
                {
                    pthread_create(&pool->m_thread_work_group[i], nullptr, work, pool);
                    pool->m_live_threads++;
                    cnt++;
                    LOG_DEBUG("Add a new thread: cur_live_threads = " + std::to_string(pool->m_live_threads));
                }
            }
            pthread_mutex_unlock(&pool->m_pool_mutex);
        }

        // ===== 销毁线程 =====
        // 条件：忙线程数*2 < 存活线程数 && 存活线程数 > 最小线程数
        if (((busy_num * 2 < live_num) || (qsize == 0 && busy_num == 0)) && live_num > pool->m_min_threads)
        {
            pthread_mutex_lock(&pool->m_pool_mutex);
            int can_exit = std::min(pool->m_op_num, live_num - pool->m_min_threads);
            pool->m_exit_threads = can_exit;
            pthread_mutex_unlock(&pool->m_pool_mutex);

            if (can_exit > 0)
            {
                LOG_DEBUG("Prepare to destroy " + std::to_string(can_exit) +
                          " threads: cur_live_threads = " + std::to_string(live_num));
                // 唤醒对应数量的线程让其退出
                for (int i = 0; i < can_exit; i++)
                {
                    pthread_cond_signal(&pool->m_empty);
                }
                // 短暂等待，让线程有时间处理退出
                usleep(200000); // 200ms
            }
        }
    }
    return nullptr;
}

void *ThreadPool::work(void *arg)
{
    ThreadPool *pool = static_cast<ThreadPool *>(arg);

    while (true)
    {
        pthread_mutex_lock(&pool->m_queue_mutex);

        while (pool->m_taskQ.empty() && !pool->m_is_shutdown)
        {
            pthread_cond_wait(&pool->m_empty, &pool->m_queue_mutex);

            // 检查是否需要退出
            if (pool->m_exit_threads > 0)
            {
                pthread_mutex_lock(&pool->m_pool_mutex);
                pool->m_exit_threads--;
                pool->m_live_threads--;
                pthread_mutex_unlock(&pool->m_pool_mutex);
                pthread_mutex_unlock(&pool->m_queue_mutex);

                thread_exit_(pool);
            }
        }

        // 线程池关闭
        if (pool->m_is_shutdown)
        {
            pthread_mutex_lock(&pool->m_pool_mutex);
            pool->m_live_threads--;
            pthread_mutex_unlock(&pool->m_pool_mutex);
            pthread_mutex_unlock(&pool->m_queue_mutex);
            thread_exit_(pool);
        }

        // 获取任务
        Task *task = pool->getTask();
        pthread_cond_signal(&pool->m_full);
        pthread_mutex_unlock(&pool->m_queue_mutex);

        // 执行任务
        pthread_mutex_lock(&pool->m_pool_mutex);
        pool->m_busy_threads++;
        pthread_mutex_unlock(&pool->m_pool_mutex);

        task->run();
        delete task;

        pthread_mutex_lock(&pool->m_pool_mutex);
        pool->m_busy_threads--;
        pthread_mutex_unlock(&pool->m_pool_mutex);
    }
    return nullptr;
}

void ThreadPool::thread_exit_(ThreadPool *pool)
{
    pthread_t tid = pthread_self();

    pthread_mutex_lock(&pool->m_pool_mutex);
    for (pthread_t &t : pool->m_thread_work_group)
    {
        if (t == tid)
        {
            t = 0;
            LOG_DEBUG("A thread is exiting...");
            break;
        }
    }
    pthread_mutex_unlock(&pool->m_pool_mutex);
    pthread_exit(nullptr);
}