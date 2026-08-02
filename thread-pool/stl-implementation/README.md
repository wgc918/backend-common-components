# STL 线程池实现

## 概述

基于 C++ 标准库接口实现的线程池，使用 `std::thread`、`std::mutex`、`std::condition_variable` 等标准库组件。

## 特点

- **纯 C++ 实现**：仅依赖 C++17 标准库，无任何系统级 API 调用
- **跨平台兼容**：可在任何支持 C++17 的平台上编译运行（Linux、macOS、Windows）
- **RAII 安全**：利用 C++ 对象生命周期自动管理资源，无需手动 `pthread_*_destroy()`
- **类型安全**：`std::thread::id`、`std::unique_lock` 等提供更强的类型检查

## 技术实现细节

### 线程管理

- 使用 `std::vector<std::thread>` 预分配线程槽位（默认构造的 `std::thread` 为 non-joinable，表示空槽位）
- 通过 `joinable()` 判断槽位是否被占用
- 线程退出时调用 `detach()` 分离线程，并将槽位重置为默认状态

### 同步机制

- `std::mutex` + `std::unique_lock` / `std::lock_guard` 实现互斥访问
- `std::condition_variable` 配合 `wait()` / `notify_one()` / `notify_all()` 实现线程间通信
- 生产者-消费者模型：`m_full` 条件变量阻塞队列满时的生产者，`m_empty` 阻塞队列空时的消费者

### 线程退出

不同于 pthread 的 `pthread_exit()`，STL 版本线程通过函数返回退出：
1. 线程在 `m_pool_mutex` 保护下找到自己的槽位
2. 调用 `detach()` 将 `std::thread` 对象与执行线程分离
3. 重置槽位为默认状态
4. 从 `work()` 函数返回，线程自然终止

### 析构等待

析构函数通过轮询 `m_live_threads` 计数器确保所有工作线程退出后再返回，避免资源泄漏。

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
./test_threadpool_stl
```

## 编译选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `BUILD_TESTS` | ON | 是否编译单元测试 |
| `ENABLE_LOGGING` | OFF（测试时 ON） | 是否启用调试日志输出 |

## 依赖

- C++17 或更高版本
- CMake 3.14+
- 支持 `-pthread` 的编译器（GCC 或 Clang）

## 文件说明

| 文件 | 说明 |
|------|------|
| `include/task.h` | 任务抽象基类 |
| `include/threadPool.h` | 线程池类声明 |
| `include/TaskDispatcher.h` | 任务分发器声明 |
| `include/simple_logger.h` | 轻量级日志宏 |
| `src/threadPool.cpp` | 线程池核心实现 |
| `src/TaskDispatcher.cpp` | 任务分发器实现 |
| `tests/test_threadpool.cpp` | 15 个单元测试用例 |