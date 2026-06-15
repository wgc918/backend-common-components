# Log 日志模块

> 高性能、线程安全的 C++ 日志管理组件

---

## 1. 模块概述

### 1.1 功能定位

本模块提供轻量级、高性能的日志记录能力，支持多级日志过滤、批量缓冲写入、自动文件轮转，适用于服务端应用的生产环境。

### 1.2 核心特性

| 特性 | 说明 |
|------|------|
| 单例模式 | 全局唯一实例，保证日志一致性 |
| 五级日志 | `DEBUG`/`INFO`/`WARNING`/`ERROR`/`FATAL` |
| 高性能 | snprintf + 4KB 栈缓冲区，零堆分配 |
| 批量缓冲 | 减少系统调用，提升吞吐量 |
| 自动轮转 | 按行数切割，防止日志过大 |
| 双输出 | 控制台/文件可独立配置 |

### 1.3 应用场景

- 服务端应用日志记录
- 多线程并发日志写入
- 高性能低延迟场景
- 生产环境日志管理

---

## 2. 设计实现

### 2.1 架构设计

```
┌──────────────────────────────────────────────┐
│              Logger (单例)                   │
├────────────┬────────────┬───────────────────┤
│  LogConfig │   Mutex    │     Buffer(4KB)  │
└─────┬──────┴─────┬──────┴─────────┬─────────┘
      │            │                │
      ▼            ▼                ▼
┌──────────────────────────────────────────────┐
│              log() 接口                      │
│  [格式化] → [级别过滤] → [输出控制]          │
└──────────────────────────────────────────────┘
```

### 2.2 核心接口

**Logger 类**

| 接口 | 功能 |
|------|------|
| `instance()` | 获取单例 |
| `init(LogConfig)` | 初始化配置 |
| `shutdown()` | 关闭并刷新 |
| `set_level(LogLevel)` | 设置日志级别 |
| `set_console(bool)` | 开关控制台输出 |

**日志宏**

| 宏 | 级别 |
|----|------|
| `LOGD(...)` | DEBUG |
| `LOGI(...)` | INFO |
| `LOGW(...)` | WARNING |
| `LOGE(...)` | ERROR |
| `LOGF(...)` | FATAL |

### 2.3 配置结构

```cpp
struct LogConfig {
    std::string file_name;   // 日志文件路径
    LogLevel    level;       // 过滤级别 (默认 INFO)
    bool        console;     // 控制台输出 (默认 true)
    size_t      max_lines;   // 单文件行数 (默认 500)
    size_t      buf_size;    // 缓冲区大小 (默认 4096)
};
```

### 2.4 关键机制

**批量刷新策略**

| 级别 | 刷新时机 |
|------|----------|
| DEBUG/INFO | 缓冲区半满(2KB) |
| WARNING+ | 立即刷新 |
| shutdown() | 强制刷新 |

**文件轮转机制**

1. 检测行数 >= `max_lines`
2. 刷新缓冲区
3. 重命名为 `{name}_{YYYYMMDD_HHMMSS}.log`
4. 新建并重置计数器

---

## 3. 使用方式

### 3.1 集成

```cpp
#include "log/include/log.h"
```

**编译**
```bash
g++ -std=c++17 -O2 your_code.cpp log/src/log.cpp -Ilog/include -o app
```

### 3.2 初始化

**控制台模式**
```cpp
Logger::instance().init(LogConfig::console_only());
```

**文件模式**
```cpp
Logger::instance().init(LogConfig::file_only("/var/log/app.log"));
```

**自定义配置**
```cpp
LogConfig cfg;
cfg.file_name = "/var/log/app.log";
cfg.level     = LogLevel::DEBUG;
cfg.console   = true;
cfg.max_lines = 1000;
Logger::instance().init(cfg);
```

### 3.3 日志输出

**直接参数**
```cpp
LOGI("user_id=", 123, ", name=", "Alice");
```

**格式化输出**
```cpp
LOGI("user_id=%d, score=%.2f", 123, 95.5);
```

### 3.4 示例代码

参见 `examples/example_singleThread.cpp` 和 `examples/example_multiThread.cpp`

---

## 4. 注意事项

### 4.1 限制

- 单条日志最大 4KB，超出截断
- 文件名建议 ≤ 255 字符
- 轮转文件需外部脚本清理

### 4.2 性能建议

- 生产环境使用 `INFO`+ 级别
- 高频日志使用 `DEBUG`/`INFO` 触发批量缓冲
- 程序退出前调用 `shutdown()`

### 4.3 最佳实践

```cpp
int main() {
    Logger::instance().init(LogConfig::file_only("/var/log/app.log"));
    
    LOGI("Application started");
    // ... 业务逻辑 ...
    
    Logger::instance().shutdown();  // 确保缓冲写入
    return 0;
}
```

---

## 5. 维护说明

### 5.1 文件管理

- 存储路径：由 `file_name` 指定
- 轮转命名：`{name}_{YYYYMMDD_HHMMSS}.log`
- 清理脚本：
  ```bash
  find /var/log/app -name "*.log" -mtime +7 -delete
  ```

### 5.2 性能测试

运行基准测试：
```bash
g++ -std=c++17 -O2 -DNDEBUG -pthread benchmark/benchmark.cpp src/log.cpp -Iinclude -o benchmark/benchmark
./benchmark/benchmark 100000 4
```

---

## 附录

### 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 2.0.0 | 2026-06-15 | snprintf 栈缓冲区优化 |
| 1.0.0 | - | 初始版本 |

### 许可证

Copyright (c) 2026 GuochengWu. All rights reserved.