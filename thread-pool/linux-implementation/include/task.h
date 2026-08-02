/*
 * Copyright (c) 2026 [GuochengWu]
 * All rights reserved.
 *
 * 定义线程池的任务基类
 *
 * 最后修改于 2026年1月19日
 *
 * 注意：任务需要在堆区构造，并且默认在线程池中自动释放
 *
 */

#pragma once

class Task
{
public:
    Task() = default;
    virtual ~Task() = default;
    virtual void run() = 0;
};