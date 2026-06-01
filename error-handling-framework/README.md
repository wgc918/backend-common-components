# Error — C++ 错误处理框架

> Header-only · C++17 · 零异常 · 小对象优化 · 即插即用

一套面向后端服务开发的轻量级错误处理框架。核心仅约 200 行，3 分钟即可集成到任意 C++17 项目中。

---

## 目录

- [快速开始](#快速开始)
- [设计理念](#设计理念)
- [架构概览](#架构概览)
- [核心组件](#核心组件)
  - [ErrorKind — 错误类型枚举](#errorkind--错误类型枚举)
  - [Error — 错误对象](#error--错误对象)
  - [Result\<T, E\> — 成功或错误](#resultt-e--成功或错误)
  - [AEF — 快速构造宏](#aef--快速构造宏)
  - [LogError — 日志输出](#logerror--日志输出)
- [扩展指南](#扩展指南)
- [性能设计](#性能设计)
- [文件清单](#文件清单)

---

## 快速开始

```cpp
#include "error/result.h"
#include "error/macros.h"
#include "error/error_utils.h"

// 1. 定义你的函数，返回 Result<T>
Result<int> Div(int a, int b) {
    if (b == 0) return AEF(InvalidArg, "division by zero");
    return a / b;
}

// 2. 调用方判断 + 处理
int main() {
    auto r = Div(10, 0);

    if (!r) {
        LogError(r.unwrap_err());   // 输出格式化日志
        return 1;
    }

    std::printf("result = %d\n", r.unwrap());
    return 0;
}
```

**输出：**
```
[2] Invalid argument (2) @ main.cpp:5 | division by zero
```

---

## 设计理念

| 原则 | 说明 |
|------|------|
| **简单直接** | `Error` 是运行时类型，不搞模板特化；`Result` 就是"要么有值要么有错"的容器 |
| **零异常** | 所有错误通过返回值显式传播，不用 `try/catch` |
| **Header-only** | 纯头文件，`#include` 即用，零链接依赖 |
| **小对象优化** | ≤ 64 字节的错误信息纯栈上存储，不触发堆分配 |
| **可扩展** | `Result<T, E>` 的 `E` 支持自定义类型，模块可定义自己专用的错误结构 |

---

## 架构概览

```
┌─────────────────────────────────────────────────────┐
│                    业务代码                          │
│  Result<T> DoWork() { ... return AEF(...); }        │
│  auto r = DoWork();                                 │
│  if (!r) { LogError(r.unwrap_err()); return; }      │
├─────────────────────────────────────────────────────┤
│  result.h   │  Result<T, E = Error> tagged union     │
├─────────────────────────────────────────────────────┤
│  error.h    │  Error: kind + location + message + chain │
├─────────────┼───────────────────────────────────────┤
│  error_kind.h  │  ErrorKind enum + kErrorTable[]    │
│  source_location.h │ SOURCE_LOC() 宏                 │
│  macros.h    │  AEF() 宏                            │
│  error_utils.h│  LogError()                          │
└─────────────────────────────────────────────────────┘
```

---

## 核心组件

### ErrorKind — 错误类型枚举

`ErrorKind` 是一个 `enum class`，预置了 9 种通用错误类型。错误码分段管理（非强制约定）：

| 区间 | 用途 | 示例 |
|------|------|------|
| 0 ~ 999 | 通用错误 | `None`, `Unknown`, `InvalidArg` |
| 1000 ~ 1999 | IO 类 | `FileNotFound`, `FilePermission` |
| 2000 ~ 2999 | 网络类 | `NetworkTimeout`, `ConnectionRefused` |
| 3000 ~ 3999 | 解析类 | `ParseError`, `InvalidFormat` |
| 10000+ | 业务自定义 | — |

每种 `ErrorKind` 在 `kErrorTable[]` 中绑定一条 `ErrorEntry`，包含错误码、描述和等级。`LookupError()` 运行时查表。

### Error — 错误对象

```cpp
class Error {
    ErrorKind       kind()      const noexcept;  // 错误类型
    SourceLocation  location()  const noexcept;  // 文件:行号:函数
    ErrorLevel      level()     const noexcept;  // 严重程度
    int             code()      const noexcept;  // 错误码
    const char*     what()      const noexcept;  // 错误描述
    const Error*    cause()     const noexcept;  // 错误链下层
    std::string     message()   const;           // 上下文消息

    Error& caused_by(Error&& cause);             // 链式附加原因
};
```

**小对象优化 (SBO)**：`Error` 内部有一个 64 字节栈缓冲区。`message()` ≤ 63 字节时完全在栈上；超出时自动 fallback 到 `std::string` 堆存储。

**错误链**：通过 `caused_by()` 可将底层错误附加为"原因"，形成因果链。`LogError()` 会自动递归打印。

```cpp
auto low  = AEF(ConnectionRefused, "connect to %s:%d failed", host, port);
auto high = AEF(NetworkTimeout, "service unavailable after retry")
                .caused_by(std::move(low));
```

### Result\<T, E\> — 成功或错误

双模板参数的 tagged union，底层为 aligned storage + placement new。

```cpp
template<typename T, typename E = Error>
class Result {
    bool is_ok()  const noexcept;
    bool is_err() const noexcept;
    explicit operator bool() const noexcept;  // if (result) ...
    T&         unwrap();                      // 解包值（非 ok 则 abort）
    const E&   unwrap_err() const;            // 解包错误
    T          value_or(T&& def) &&;           // 错误时取默认值
};
```

**自定义错误类型**：将 `E` 替换为模块自定义结构即可。

```cpp
struct NetError { int http_code; std::string body; };
Result<std::string, NetError> FetchUrl(const std::string& url) {
    if (failed) return NetError{503, "Service Unavailable"};
    return "response body";
}
```

### AEF — 快速构造宏

一行构造 `Error` 并自动捕获 `__FILE__` / `__LINE__` / `__FUNCTION__`。

```cpp
#define AEF(kind, fmt, ...) \
    Error(ErrorKind::kind, SOURCE_LOC(), fmt, ##__VA_ARGS__)

// 用法
return AEF(FileNotFound, "config '%s' not found in %s", name, dir);
return AEF(ParseError, "unexpected token at line %d", line);
```

### LogError — 日志输出

统一格式输出到 `stderr`，自动递归打印错误链。

```
[3] File not found (1001) @ main.cpp:30 | cannot open 'config.ini'
[3] Network timeout (2001) @ client.cpp:88 | service call failed
  caused by: Connection refused
```

---

## 扩展指南

### 添加新错误类型（两步）

**第一步**：在 `include/error/error_kind.h` 的 `ErrorKind` 枚举中添加：

```cpp
enum class ErrorKind : int {
    // ... 现有值 ...
    MyCustomError,   // 新增
};
```

**第二步**：在 `kErrorTable[]` 中添加对应条目：

```cpp
{ ErrorKind::MyCustomError, 5001, "Custom error occurred", ErrorLevel::Error },
```

### 自定义错误类型（模块级）

定义你的错误结构体，然后作为 `Result<T, E>` 的第二个模板参数：

```cpp
// 数据库模块
struct DbError { int sql_code; std::string detail; };
Result<User, DbError> QueryUser(int id);

// 使用
auto r = QueryUser(42);
if (!r) {
    auto& e = r.unwrap_err();
    std::fprintf(stderr, "SQL error %d: %s\n", e.sql_code, e.detail.c_str());
}
```

### 序列化支持

以下为 `nlohmann/json` 示例，可按需适配其他序列化库：

```cpp
inline nlohmann::json ToJson(const Error& err) {
    return {
        {"kind",   static_cast<int>(err.kind())},
        {"code",   err.code()},
        {"level",  static_cast<int>(err.level())},
        {"what",   err.what()},
        {"detail", err.message()},
        {"file",   err.location().file},
        {"line",   err.location().line},
    };
}
```

---

## 性能设计

| 场景 | 策略 | 开销 |
|------|------|------|
| 成功路径 | `Result` 仅一个 `bool` 判断 | 零分配，1 次分支 |
| 短错误信息 (≤63B) | 栈缓冲区 `char[64]` | 零堆分配 |
| 长错误信息 (>63B) | 自动 `std::string` | 1 次堆分配（仅错误路径） |
| 错误拷贝 | 深拷贝，递归复制链 | 与错误链长度成正比 |
| 错误移动 | `= default`，逐成员移动 | 接近零开销 |
| `Result` 大小 | `max(sizeof(T), sizeof(E)) + bool + padding` | 与手写 `variant` 一致 |

---

## 文件清单

```
include/error/
├── error_kind.h          ErrorKind 枚举 + ErrorLevel + kErrorTable 查表
├── source_location.h     SourceLocation 结构体 + SOURCE_LOC() 宏
├── error.h               Error 类（核心错误对象，SBO + 错误链）
├── result.h              Result<T, E>（tagged union，默认 E = Error）
├── macros.h              AEF() 快速构造宏
└── error_utils.h         LogError() 日志工具函数

examples/
└── error_example.cpp     完整使用示例（5 个测试场景）
```

---

以上设计可立即投入生产使用。
