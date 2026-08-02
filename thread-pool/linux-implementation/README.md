# Linux / pthread 线程池实现

## 概述

基于 Linux 原生 pthread 接口实现的线程池，使用 `pthread_create`、`pthread_mutex_t`、`pthread_cond_t` 等 POSIX 线程 API。

## 特点

- **原生性能**：直接使用 Linux 内核支持的 pthread 接口，无 C++ 运行时抽象层开销
- **精细控制**：可精确控制线程属性、调度策略等底层参数
- **轻量级**：不依赖 C++ 标准库线程组件，适合嵌入式或资源受限环境
- **成熟稳定**：pthread 接口经过数十年验证，行为明确且可预测

## 技术实现细节

### 线程管理

- 使用 `std::vector<pthread_t>` 预分配线程槽位（初始化为 0 表示空槽位）
- 通过 `pthread_create()` 创建线程，槽位存储线程 ID
- 线程退出时通过 `pthread_self()` 查找自身槽位，置 0 后调用 `pthread_exit()` 终止

### 同步机制

- `pthread_mutex_t` 配合 `pthread_mutex_lock()` / `pthread_mutex_unlock()` 实现互斥
- `pthread_cond_t` 配合 `pthread_cond_wait()` / `pthread_cond_signal()` / `pthread_cond_broadcast()` 实现条件同步
- 生产者-消费者模型：`m_full` 条件变量阻塞队列满时的生产者，`m_empty` 阻塞队列空时的消费者

### 线程退出

使用 POSIX 标准的 `pthread_exit()` 终止线程：
1. 线程在 `m_pool_mutex` 保护下找到自己的槽位
2. 将槽位重置为 0（标记为空闲）
3. 调用 `pthread_exit(nullptr)` 终止当前线程

### 析构清理

析构函数按顺序：
1. 设置关闭标志
2. 广播唤醒所有等待线程
3. `pthread_join()` 等待管理者线程退出
4. `pthread_cond_destroy()` / `pthread_mutex_destroy()` 销毁同步原语

## 构建

```bash
mkdir build && cd build
cmake ..
make
```

## 测试

```bash
cd build
ctest --output-on-failure
```

或直接运行测试二进制：

```bash
./test_threadpool_linux
```

## 编译选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `BUILD_TESTS` | ON | 是否编译单元测试 |
| `ENABLE_LOGGING` | OFF（测试时 ON） | 是否启用调试日志输出 |

## 依赖

- Linux 操作系统
- C++17 或更高版本（用于 `std::queue`、`std::vector` 等容器）
- CMake 3.14+
- pthread 库（系统自带）

## 与 STL 版本的差异

| 方面 | Linux 实现 | STL 实现 |
|------|-----------|----------|
| 线程创建 | `pthread_create()` | `std::thread` 构造 |
| 线程退出 | `pthread_exit()` | 函数返回 + `detach()` |
| 互斥锁 | `pthread_mutex_t` | `std::mutex` |
| 条件变量 | `pthread_cond_t` | `std::condition_variable` |
| 锁管理 | 手动 lock/unlock | RAII (`lock_guard`, `unique_lock`) |
| 资源清理 | 手动 `_destroy()` | 自动析构 |
| 跨平台 | ❌ 仅 Linux | ✅ 跨平台 |
| 性能 | 理论上更优（无抽象层） | 略低（但差异极小） |

## 文件说明

| 文件 | 说明 |
|------|------|
| `include/task.h` | 任务抽象基类 |
| `include/threadPool.h` | 线程池类声明（pthread 版本） |
| `include/TaskDispatcher.h` | 任务分发器声明 |
| `include/simple_logger.h` | 轻量级日志宏 |
| `src/threadPool.cpp` | 线程池核心实现（pthread 版本） |
| `src/TaskDispatcher.cpp` | 任务分发器实现 |
| `tests/test_threadpool.cpp` | 15 个单元测试用例 |