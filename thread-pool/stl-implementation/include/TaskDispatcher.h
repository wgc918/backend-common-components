/*
 * Copyright (c) 2026 [GuochengWu]
 * All rights reserved.
 *
 * 实现线程池任务分发的轻量级封装
 *
 * 最后修改于 2026年1月19日
 */

#pragma once

#include "task.h"
#include "threadPool.h"

class TaskDispatcher
{
private:
    ThreadPool &pool;

public:
    explicit TaskDispatcher(ThreadPool &pool) : pool(pool) {}
    ~TaskDispatcher() = default;

    void task_add(Task *task);
};