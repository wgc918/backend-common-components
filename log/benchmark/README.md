# Log 模块性能基准测试

> Logger 模块的性能对比测试工具

---

## 概述

本目录包含 Log 模块的性能基准测试套件，用于验证和对比不同实现方案的性能差异。

## 测试项目

| 测试项 | 说明 |
|--------|------|
| 格式化性能 | stringstream vs snprintf 栈缓冲区 |
| 写入吞吐量 | 不同级别日志的每秒写入条数 |
| 内存分配 | 堆分配次数与内存峰值对比 |
| 并发写入 | 多线程环境下的性能表现 |

## 编译与运行

### 编译

```bash
g++ -std=c++17 -O2 -DNDEBUG -pthread \
    benchmark.cpp ../src/log.cpp \
    -I../include -o benchmark
```

### 运行

```bash
# 默认：10万次迭代，4线程
./benchmark

# 自定义参数
./benchmark <iterations> <threads>
./benchmark 100000 4
```

## 测试结果解读

### 输出示例

```
============================================================
  Logger 性能基准测试
  Iterations: 100000  |  Threads: 4
============================================================

--- 1. 纯格式化性能对比 ---
[stringstream format] 100000 iterations: 45.23 ms (0.45 us/op)
[snprintf   format] 100000 iterations: 12.15 ms (0.12 us/op)
  => snprintf 比 stringstream 快约 3-4 倍

--- 2. 内存分配对比 ---
[Memory stringstream] 100000 ops: 200000 allocations, 8192 KB total
[Memory snprintf    ] 100000 ops: 0 allocations, 0 KB total

--- 3. 日志写入吞吐量 ---
[Logger  throughput] 100000 iterations: 85.32 ms (117200 ops/sec)
[Logger  batched  ] 100000 iterations: 32.15 ms (311000 ops/sec)

--- 4. 多线程并发写入 ---
[Logger  MT 4T ] 100000 total ops: 125.67 ms (795700 ops/sec)
[Logger  MT 1T ] 25000 total ops: 28.34 ms (88200 ops/sec)
```

### 关键指标

| 指标 | 目标值 |
|------|--------|
| 单线程吞吐量 | > 100,000 ops/sec |
| 多线程吞吐量 | > 500,000 ops/sec (4线程) |
| snprintf 加速比 | 3-5x vs stringstream |
| 堆分配次数 | 0 (DEBUG/INFO级别) |

## 测试原理

### 格式化对比

- **stringstream**：传统实现，使用堆分配的字符串流
- **snprintf**：优化实现，使用 4KB 栈缓冲区，零堆分配

### 批量缓冲测试

DEBUG/INFO 级别触发批量缓冲策略，仅在缓冲区半满(2KB)时刷新，大幅减少系统调用。

### 并发测试

使用 `std::mutex` 保证线程安全，测试多线程竞争场景下的吞吐量。

## 使用建议

1. **开发阶段**：运行测试验证性能回归
2. **版本发布**：确认关键指标达标
3. **性能调优**：对比不同实现方案的效果

---

## 附录

### 测试环境要求

- C++17 或更高版本
- POSIX 线程库 (-pthread)
- 优化编译 (-O2 -DNDEBUG)