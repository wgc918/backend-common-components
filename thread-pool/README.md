# Thread Pool — 双实现方案

## 概述

本目录提供两种功能逻辑完全一致的线程池实现方案：

| 实现 | 目录 | 底层技术 | 特点 |
|------|------|----------|------|
| **STL 实现** | `stl-implementation/` | C++ 标准库（`std::thread`, `std::mutex`, `std::condition_variable`） | 跨平台，纯 C++，无系统依赖 |
| **Linux 实现** | `linux-implementation/` | Linux 原生接口（`pthread`） | 高性能，Linux 原生，无 C++ 运行时开销 |

两种实现对外暴露 **完全一致的 API 接口**，调用方可以无缝切换。

## 目录结构

```
thread-pool/
├── README.md                         # 本文件
├── include/                          # 原始实现（不移除，保持兼容）
├── src/
├── stl-implementation/               # STL 版本
│   ├── include/
│   │   ├── task.h                    # 任务基类
│   │   ├── threadPool.h              # 线程池头文件
│   │   ├── TaskDispatcher.h          # 任务分发器
│   │   └── simple_logger.h           # 轻量日志
│   ├── src/
│   │   ├── threadPool.cpp            # 线程池实现
│   │   └── TaskDispatcher.cpp        # 任务分发器实现
│   ├── tests/
│   │   └── test_threadpool.cpp       # 单元测试
│   ├── CMakeLists.txt
│   └── README.md
└── linux-implementation/             # Linux/pthread 版本
    ├── include/
    ├── src/
    ├── tests/
    ├── CMakeLists.txt
    └── README.md
```

## 核心功能

两种实现均提供以下完整功能：

- **线程创建与管理**：基于最小/最大线程数配置，自动管理线程生命周期
- **任务队列管理**：线程安全的生产者-消费者模型，支持队列容量限制
- **任务提交接口**：`submit(Task*)` 统一提交接口
- **动态扩缩容**：管理者线程定期监控，根据负载自动增减工作线程
- **安全启停控制**：支持优雅关闭，确保所有已提交任务执行完毕
- **资源回收**：析构时自动销毁所有线程和同步资源
- **线程安全**：完整的互斥锁和条件变量保护

## 统一 API

```cpp
class ThreadPool {
public:
    // 构造函数：min=最小线程数, max=最大线程数, op_num=每次扩缩容操作数, cap=队列容量
    ThreadPool(int min, int max, int op_num, size_t cap);

    // 禁止拷贝与移动
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    // 提交任务（任务在堆上创建，线程池负责释放）
    void submit(Task *task);

    // 设置管理者线程巡检频率（秒）
    void set_check_freq(int check_freq);

    // 查询存活线程数
    int get_live_threads();

    // 查询忙碌线程数
    int get_busy_threads();
};

// 任务基类
class Task {
public:
    virtual ~Task() = default;
    virtual void run() = 0;
};

// 任务分发器（轻量封装）
class TaskDispatcher {
public:
    explicit TaskDispatcher(ThreadPool &pool);
    void task_add(Task *task);
};
```

## 使用方法

### STL 版本

```cpp
#include "task.h"
#include "threadPool.h"

// 创建自定义任务
class MyTask : public Task {
    void run() override {
        // 任务逻辑
    }
};

int main() {
    // 创建线程池：最小4线程，最大16线程，每次扩缩2线程，队列容量100
    ThreadPool pool(4, 16, 2, 100);

    // 提交任务
    pool.submit(new MyTask());

    return 0; // 析构时自动等待所有任务完成
}
```

### Linux 版本

```cpp
#include "task.h"
#include "threadPool.h"

// 使用方式与 STL 版本完全一致
int main() {
    ThreadPool pool(4, 16, 2, 100);
    pool.submit(new MyTask());
    return 0;
}
```

## 切换实现

只需修改 `#include` 路径和链接库：

| 场景 | include 路径 | 链接库 |
|------|-------------|--------|
| STL 版本 | `stl-implementation/include/` | `threadpool_stl` |
| Linux 版本 | `linux-implementation/include/` | `threadpool_linux` |

## 构建与测试

### STL 版本

```bash
cd stl-implementation
mkdir build && cd build
cmake ..
make
ctest --output-on-failure
```

### Linux 版本

```bash
cd linux-implementation
mkdir build && cd build
cmake ..
make
ctest --output-on-failure
```

## 设计说明

### 架构设计

两种实现均采用「管理者线程 + 工作线程组」双层架构：

```
┌─────────────────────────────────────┐
│            submit(task)             │
└───────────────┬─────────────────────┘
                ▼
┌─────────────────────────────────────┐
│          任务队列 (m_taskQ)          │
│       容量限制 (m_capacity)          │
└───────────────┬─────────────────────┘
                │
    ┌───────────┼───────────┐
    ▼           ▼           ▼
┌──────┐  ┌──────┐     ┌──────┐
│Worker│  │Worker│ ... │Worker│   工作线程组
└──────┘  └──────┘     └──────┘
                ▲
                │ 动态扩缩容
        ┌───────┴───────┐
        │   管理者线程    │  定期巡检
        └───────────────┘
```

### 扩缩容策略

- **扩容**：当任务队列长度 > 存活线程数 且 存活线程数 < 最大线程数时，按 `op_num` 步长添加线程
- **缩容**：当 忙碌线程数 × 2 < 存活线程数 且 存活线程数 > 最小线程数时，按 `op_num` 步长销毁线程

### 技术差异

| 特性 | STL 实现 | Linux 实现 |
|------|----------|------------|
| 线程创建 | `std::thread` 构造函数 | `pthread_create()` |
| 互斥锁 | `std::mutex` + `std::unique_lock` | `pthread_mutex_t` |
| 条件变量 | `std::condition_variable` | `pthread_cond_t` |
| 线程退出 | 函数返回 + `detach()` | `pthread_exit()` |
| 休眠 | `std::this_thread::sleep_for()` | `sleep()` / `usleep()` |
| 线程 ID | `std::thread::id` | `pthread_t` |
| 线程槽管理 | `joinable()` 检测 | `== 0` 检测 |
| 跨平台 | ✅ 是 | ❌ 仅 Linux |

## 许可证

MIT License — 详见项目根目录 LICENSE 文件。