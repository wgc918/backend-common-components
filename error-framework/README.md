# EF — C++ 错误处理框架

## 目录

- [1. 概述](#1-概述)
- [2. 设计理念](#2-设计理念)
- [3. 目录结构](#3-目录结构)
- [4. 核心组件](#4-核心组件)
  - [4.1 SourceLocation — 源码位置](#41-sourcelocation--源码位置)
  - [4.2 Severity — 错误严重程度](#42-severity--错误严重程度)
  - [4.3 ErrorInfo — 错误元数据](#43-errorinfo--错误元数据)
  - [4.4 内置错误码](#44-内置错误码)
  - [4.5 Error — 运行时错误对象](#45-error--运行时错误对象)
  - [4.6 Result — 成功/错误二选一容器](#46-result--成功错误二选一容器)
  - [4.7 外部错误注册表](#47-外部错误注册表)
  - [4.8 工具函数](#48-工具函数)
- [5. 便捷宏](#5-便捷宏)
  - [5.1 EF_SOURCE_LOC](#51-ef_source_loc)
  - [5.2 EF_ERROR / EF_MAKE](#52-ef_error--ef_make)
  - [5.3 EF_TRY](#53-ef_try)
  - [5.4 EF_CHECK](#54-ef_check)
  - [5.5 EF_RETURN_IF](#55-ef_return_if)
  - [5.6 EF_EXPECT_OR](#56-ef_expect_or)
- [6. 模块自定义错误](#6-模块自定义错误)
  - [6.1 X-Macro 模式（推荐）](#61-x-macro-模式推荐)
  - [6.2 手动定义模式](#62-手动定义模式)
- [7. 使用示例](#7-使用示例)
  - [7.1 基础用法](#71-基础用法)
  - [7.2 错误链](#72-错误链)
  - [7.3 value_or 默认值](#73-value_or-默认值)
  - [7.4 void Result 与 EF_CHECK](#74-void-result-与-ef_check)
  - [7.5 模块错误集成](#75-模块错误集成)
  - [7.6 错误日志与终止](#76-错误日志与终止)
- [8. 配置与集成](#8-配置与集成)
- [9. 内存模型](#9-内存模型)
- [10. API 参考](#10-api-参考)
- [11. 常见问题解答](#11-常见问题解答)
- [12. 扩展指南](#12-扩展指南)

---

## 1. 概述

EF（Error Framework）是一个面向 C++17 的轻量级、零依赖错误处理框架。它借鉴了 Rust 语言中 `Result<T, E>` 和 `?` 操作符的设计思想，提供 tagged union 风格的 `Result<T, E>` 容器、带错误链的 `Error` 对象以及一系列便捷宏，让 C++ 的错误处理从层级混乱的 `if (ret != 0)` 和异常满天飞中解脱出来。

框架仅由 **7 个头文件** 组成，无需编译，直接 `#include` 即可使用。

**核心目标：**

- **显式错误传播**：通过 `Result<T, E>` 让错误路径在函数签名中可见
- **零堆分配路径**：短消息（≤ 64 字节）完全栈上操作
- **模块化扩展**：业务模块可在不修改框架源码的情况下定义自有错误码
- **C++17 兼容**：无需 C++20/23 特性

---

## 2. 设计理念

| 理念 | 说明 |
|------|------|
| **错误即值** | 错误不是异常，而是返回值的另一种形态。函数返回 `Result<T, E>`，调用方必须显式处理错误分支 |
| **SBO 优先** | 错误上下文消息 ≤ 64 字节时使用栈缓冲区（Small Buffer Optimization），避免堆分配 |
| **深拷贝语义** | `Error` 的拷贝构造/赋值递归复制整个错误链，避免共享指针带来的所有权歧义 |
| **int 错误码** | 使用 `int` 而非 `enum class` 作为错误码，支持业务模块自由扩展（10000+ 号段） |
| **编译期查表** | 内置错误元数据表为 `constexpr` 数组，查表函数 `LookupInTable` 为 `constexpr`，零运行时初始化开销 |
| **禁止拷贝** | `Result` 禁止拷贝构造和拷贝赋值，仅支持移动，避免隐式数据重复，语义清晰 |

---

## 3. 目录结构

```
error-framework/
├── include/
│   └── ef/
│       ├── ef.h                  # 框架汇总头文件（业务模块唯一需要 #include 的入口）
│       ├── source_location.h     # 源码位置信息结构 + EF_SOURCE_LOC 宏
│       ├── error_kind.h          # Severity 枚举、ErrorInfo 结构、内置错误码、查表函数
│       ├── error.h               # Error 运行时错误对象（SBO + 错误链）
│       ├── result.h              # Result<T, E> tagged union + Result<void> 特化
│       ├── error_def.h           # 模块自定义错误码生成宏（EF_GEN_ERROR_CODES / EF_GEN_ERROR_TABLE）
│       ├── macros.h              # 便捷宏（EF_ERROR / EF_TRY / EF_CHECK / EF_RETURN_IF / EF_EXPECT_OR）
│       └── utils.h               # 工具函数（LogError / ErrorString / Panic）
├── examples/
│   ├── basic/
│   │   └── main.cpp              # 基础使用示例（内置错误码）
│   └── module/
│       ├── db_errors.h           # 模块自定义错误定义示例
│       └── main.cpp              # 模块错误集成示例
└── README.md
```

**头文件依赖关系：**

```
source_location.h
       │
       ▼
 error_kind.h  ──►  error.h  ──►  result.h
       │                             │
       ▼                             ▼
 error_def.h                     macros.h
                                     │
                                     ▼
                                  utils.h
```

---

## 4. 核心组件

### 4.1 SourceLocation — 源码位置

**定义文件：** `ef/source_location.h`

```cpp
struct SourceLocation {
    const char* file;      // 源码文件名 (__FILE__)
    int         line;      // 源码行号 (__LINE__)
    const char* function;  // 函数名 (__FUNCTION__)
};
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `file` | `const char*` | 发生错误的源文件名 |
| `line` | `int` | 发生错误的行号 |
| `function` | `const char*` | 发生错误的函数名 |

通常不需要手动构造，由 `EF_SOURCE_LOC()` 宏在构造 `Error` 时自动填充。

**宏 `EF_SOURCE_LOC()`**：自动捕获当前源码位置。必须在函数体内调用（依赖 `__FUNCTION__`）。

```cpp
auto loc = EF_SOURCE_LOC();
// loc.file == "main.cpp", loc.line == 42, loc.function == "main"
```

---

### 4.2 Severity — 错误严重程度

**定义文件：** `ef/error_kind.h`

```cpp
enum class Severity : int {
    Debug = 0,  // 调试期输出，不构成真正的错误
    Info  = 1,  // 运行时提示信息
    Warn  = 2,  // 警告，操作可继续但需关注
    Error = 3,  // 错误，当前操作失败
    Fatal = 4,  // 致命错误，程序可能无法继续
};
```

| 级别 | 值 | 推荐场景 |
|------|-----|---------|
| `Debug` | 0 | 开发调试期的跟踪信息 |
| `Info` | 1 | 运行时的一般性提示 |
| `Warn` | 2 | 非预期但可恢复的情况（如重复插入） |
| `Error` | 3 | 当前操作失败（如连接超时、文件未找到） |
| `Fatal` | 4 | 程序无法继续（如内存耗尽、内部状态异常） |

---

### 4.3 ErrorInfo — 错误元数据

**定义文件：** `ef/error_kind.h`

```cpp
struct ErrorInfo {
    int         code;       // 错误码
    const char* name;       // 错误名称（如 "InvalidArg"）
    const char* message;    // 人类可读描述
    Severity    severity;   // 严重程度
};
```

每种错误码对应一条 `ErrorInfo`，存储在编译期常量表 `kBuiltinErrorTable[]` 中。查表函数：

```cpp
template <size_t N>
inline constexpr const ErrorInfo* LookupInTable(const ErrorInfo (&table)[N], int code) noexcept;

inline const ErrorInfo* LookupError(int code) noexcept;
```

- `LookupError(code)` 先查内置表，未命中再查外部注册表，最终回退到 `kUnknown`，**保证非空返回**。

---

### 4.4 内置错误码

**定义文件：** `ef/error_kind.h`，命名空间 `ef::errors`

错误码分段约定：

| 号段 | 用途 |
|------|------|
| `0` | 成功（不是错误） |
| `1 ~ 99` | 通用逻辑错误 |
| `100 ~ 199` | IO 类错误 |
| `200 ~ 299` | 网络类错误 |
| `300 ~ 399` | 解析/格式类错误 |
| `400 ~ 499` | 资源类错误 |
| `500 ~ 999` | 保留扩展 |
| `10000+` | 业务模块自定义 |

**完整内置错误码列表：**

| 常量 | 值 | 名称 | 描述 | 严重程度 |
|------|-----|------|------|---------|
| `kOk` | 0 | `Ok` | Success | `Debug` |
| `kUnknown` | 1 | `Unknown` | Unknown error | `Error` |
| `kInvalidArg` | 2 | `InvalidArg` | Invalid argument | `Warn` |
| `kNotSupported` | 3 | `NotSupported` | Operation not supported | `Error` |
| `kNotFound` | 100 | `NotFound` | Resource not found | `Error` |
| `kPermission` | 101 | `Permission` | Permission denied | `Error` |
| `kAlreadyExists` | 102 | `AlreadyExists` | Resource already exists | `Warn` |
| `kEOF` | 103 | `EOF` | Unexpected end of file | `Error` |
| `kTimeout` | 200 | `Timeout` | Operation timed out | `Error` |
| `kConnectionRefused` | 201 | `ConnectionRefused` | Connection refused | `Error` |
| `kConnectionReset` | 202 | `ConnectionReset` | Connection reset by peer | `Error` |
| `kParseError` | 300 | `ParseError` | Parse error | `Warn` |
| `kInvalidFormat` | 301 | `InvalidFormat` | Invalid format | `Warn` |
| `kOutOfMemory` | 400 | `OutOfMemory` | Out of memory | `Fatal` |
| `kOverflow` | 401 | `Overflow` | Buffer overflow | `Error` |
| `kOutOfRange` | 402 | `OutOfRange` | Index out of range | `Error` |
| `kEmpty` | 403 | `Empty` | Container is empty | `Error` |
| `kFull` | 404 | `Full` | Container is full | `Error` |
| `kInternal` | 500 | `Internal` | Internal error | `Fatal` |
| `kUnimplemented` | 501 | `Unimplemented` | Function not implemented | `Error` |

---

### 4.5 Error — 运行时错误对象

**定义文件：** `ef/error.h`

```cpp
class Error {
public:
    static constexpr size_t kSboSize = 64;
    // 构造
    template <typename... Args>
    Error(int code, SourceLocation loc, const char* fmt, Args&&... args);
    Error(int code, SourceLocation loc);
    Error(const Error& other);                    // 深拷贝
    Error(Error&&) noexcept = default;            // 移动
    Error& operator=(const Error& other);         // 深拷贝赋值
    Error& operator=(Error&&) noexcept = default; // 移动赋值
    // 错误链
    Error& caused_by(Error&& cause);
    bool    has_cause() const noexcept;
    // 访问器
    int             code()        const noexcept;
    SourceLocation  location()    const noexcept;
    Severity        severity()    const noexcept;
    const char*     what()        const noexcept;
    const char*     description() const noexcept;
    const Error*    cause()       const noexcept;
    bool            is_ok()       const noexcept;
    std::string     message()     const;          // 上下文消息
    bool            has_message() const noexcept;
    std::string     to_string()   const;           // 完整格式化输出
};
```

**关键特性：**

1. **SBO 栈优化**：`kSboSize = 64` 字节栈缓冲区。格式化结果 ≤ 63 字节时完全栈上操作，零堆分配；超出时自动使用 `std::string` 堆存储。

2. **深拷贝语义**：拷贝构造和拷贝赋值递归复制整个错误链（`cause_`），保证所有权独立。

3. **错误链**：`caused_by(Error&&)` 接受右值，将底层错误附加为当前错误的原因。通过 `cause()` 可遍历整个链。

4. **访问器区分**：
   - `what()` — 错误名称（来自注册表的静态名称，如 `"NotFound"`）
   - `description()` — 错误描述（来自注册表的静态描述，如 `"Resource not found"`）
   - `message()` — 上下文消息（构造时传入的动态信息，如 `"cannot open 'config.ini'"`）
   - `to_string()` — 完整格式化字符串，包含错误链

**`to_string()` 输出格式：**

```
NotFound (100) @ main.cpp:30 | cannot open 'config.ini'
  caused by: ConnectionRefused (201) @ net.cpp:44 | connect to 10.0.0.1:8080
```

---

### 4.6 Result — 成功/错误二选一容器

**定义文件：** `ef/result.h`

#### Result<T, E> 通用版本

```cpp
template <typename T, typename E = Error>
class Result {
public:
    Result(const T& val);       // 从成功值构造（拷贝）
    Result(T&& val) noexcept;   // 从成功值构造（移动）
    Result(const E& err);       // 从错误构造（拷贝）
    Result(E&& err) noexcept;   // 从错误构造（移动）
    Result(Result&&) noexcept;  // 移动构造
    // 禁止拷贝
    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;
    // 状态判断
    bool is_ok()  const noexcept;
    bool is_err() const noexcept;
    explicit operator bool() const noexcept;  // 隐式 bool 转换
    // 解包
    const T& unwrap()      const;
    T&       unwrap();
    T        unwrap_move();
    const E& unwrap_err()  const;
    E&       unwrap_err();
    T        value_or(T&& def)      &&;
    T        value_or(const T& def) const &;
    // 指针访问
    T*       operator->();
    const T* operator->() const;
    T&       operator*();
    const T& operator*()  const;
};
```

**关键特性：**

- **Tagged Union**：底层使用 `alignas` + `unsigned char buf_[]` + placement new 实现，T 和 E 共用同一块存储区。
- **禁止拷贝**：`Result` 独占所有权，拷贝语义被显式禁止。需要传递 Result 时请使用 `std::move`。
- **`explicit operator bool`**：支持 `if (!r)`, `if (r)` 等自然语法。
- **`unwrap()`**：若当前持有错误则调用 `std::abort()` 终止，调用方需先检查。
- **`value_or()`**：错误时返回默认值，适合降级逻辑。

#### Result<void, E> 特化版本

```cpp
template <typename E>
class Result<void, E> {
public:
    Result();                   // 成功，无返回值
    Result(const E& err);
    Result(E&& err) noexcept;
    Result(Result&&) noexcept;
    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;
    bool is_ok()  const noexcept;
    bool is_err() const noexcept;
    explicit operator bool() const noexcept;
    const E& unwrap_err() const;
    E&       unwrap_err();
};
```

`Result<void>` 不持有成功值，仅表示成功或错误状态。成功时返回 `{}`：

```cpp
Result<void> Init() {
    if (!config.Load()) return EF_ERROR(kNotFound, "config missing");
    return {};  // 成功
}
```

---

### 4.7 外部错误注册表

**定义文件：** `ef/error.h`

```cpp
using ExternalLookupFn = const ErrorInfo* (*)(int code);
void SetExternalLookup(ExternalLookupFn fn);
```

模块通过 `SetExternalLookup` 注册回调，使框架的 `LookupError()` 和 `LogError()` 能够正确识别模块自定义错误码的名称、描述和严重程度。

**注册时机：** 模块初始化阶段（如 `main()` 开头），调用一次即可。

```cpp
ef::SetExternalLookup(db::LookupError);
```

> **注意：** 外部注册表是全局单例。如果多个模块需要注册，调用方应在入口层封装一个组合回调，依次查询各模块。

---

### 4.8 工具函数

**定义文件：** `ef/utils.h`

| 函数 | 签名 | 说明 |
|------|------|------|
| `LogError` | `void LogError(const Error& err, const char* prefix = "")` | 以统一格式将错误输出到 `stderr`，递归打印错误链 |
| `ErrorString` | `std::string ErrorString(const Error& err)` | 返回格式化的错误字符串（等价于 `err.to_string()`） |
| `Panic` | `[[noreturn]] void Panic(const Error& err)` | 输出致命错误日志后调用 `std::abort()` 终止程序 |

**LogError 输出格式：**

无错误链：
```
[Warn] NotFound (100) @ main.cpp:30 | cannot open 'config.ini'
```

有错误链：
```
[Error] Timeout (200) @ client.cpp:88 | request failed after 3 retries
  -> caused by: ConnectionRefused (201) @ net.cpp:44 | connect to 10.0.0.1:8080
```

---

## 5. 便捷宏

### 5.1 EF_SOURCE_LOC

**定义文件：** `ef/source_location.h`

```cpp
#define EF_SOURCE_LOC() ::ef::SourceLocation{ __FILE__, __LINE__, __FUNCTION__ }
```

自动捕获当前源码位置。通常在宏内部使用，用户一般不需要直接调用。

---

### 5.2 EF_ERROR / EF_MAKE

**定义文件：** `ef/macros.h`

```cpp
#define EF_ERROR(code, fmt, ...) ::ef::Error(code, EF_SOURCE_LOC(), fmt, ##__VA_ARGS__)
#define EF_MAKE(code, fmt, ...)  EF_ERROR(code, fmt, ##__VA_ARGS__)
```

快速构造 `Error` 对象，自动捕获源码位置。支持 `printf` 风格格式化。

```cpp
// 带上下文消息
return EF_ERROR(errors::kNotFound, "cannot open '%s'", path);

// 仅错误码（无附加上下文）
return EF_ERROR(errors::kInternal);

// EF_MAKE 是 EF_ERROR 的语义化别名
return EF_MAKE(errors::kTimeout, "request took %d ms", elapsed);
```

> **注意：** 格式化结果 ≤ 63 字节时完全栈上操作，零堆分配。超出 `kSboSize` 时会自动切换到堆分配。

---

### 5.3 EF_TRY

**定义文件：** `ef/macros.h`

```cpp
#define EF_TRY(expr)                                      \
    ({                                                    \
        auto _ef_try_result = (expr);                     \
        if (!_ef_try_result) {                            \
            return _ef_try_result.unwrap_err();           \
        }                                                 \
        _ef_try_result.unwrap_move();                     \
    })
```

Rust `?` 操作符风格：解包 `Result<T>`，成功时移出值，失败时自动向外传播错误。

**前提条件：**
- 当前函数返回类型为 `Result<U, E>`，且 `E` 与 `expr` 的错误类型兼容
- `expr` 返回 `Result<T, E>`

```cpp
Result<ServerConfig> LoadConfig(const std::string& path) {
    auto content = EF_TRY(ReadFile(path));   // 失败时自动 return 错误
    auto config  = EF_TRY(ParseConfig(content));
    return config;
}
```

> **展开效果：** `auto _ef_tmp = (expr); if (!_ef_tmp) return _ef_tmp.unwrap_err();`

---

### 5.4 EF_CHECK

**定义文件：** `ef/macros.h`

```cpp
#define EF_CHECK(expr)                                    \
    do {                                                  \
        auto _ef_check_result = (expr);                   \
        if (!_ef_check_result) {                          \
            return _ef_check_result.unwrap_err();         \
        }                                                 \
    } while (false)
```

检查 `Result<void>` 是否成功，失败则提前返回。适用于 void 返回值的函数或仅需检查副作用的场景。

```cpp
Result<void> InitServer() {
    EF_CHECK(load_config());    // 失败时提前返回
    EF_CHECK(connect_db());
    EF_CHECK(start_server());
    return {};
}
```

---

### 5.5 EF_RETURN_IF

**定义文件：** `ef/macros.h`

```cpp
#define EF_RETURN_IF(cond, code, fmt, ...)                \
    do {                                                  \
        if (cond) {                                       \
            return EF_ERROR(code, fmt, ##__VA_ARGS__);    \
        }                                                 \
    } while (false)
```

条件为真时构造错误并返回。适合参数校验等前置检查。

```cpp
Result<void> SetPort(int port) {
    EF_RETURN_IF(port <= 0,    errors::kInvalidArg, "port must be > 0, got %d", port);
    EF_RETURN_IF(port > 65535, errors::kInvalidArg, "port too large: %d", port);
    // ... 实际逻辑
    return {};
}
```

---

### 5.6 EF_EXPECT_OR

**定义文件：** `ef/macros.h`

```cpp
#define EF_EXPECT_OR(cond, code, fmt, ...) \
    do { if (!(cond)) { return EF_ERROR(code, fmt, ##__VA_ARGS__); } } while (false)
```

"期望条件成立，否则报错"——条件为假时构造错误并返回。

```cpp
Result<std::string> ReadLine(File& f) {
    EF_EXPECT_OR(f.is_open(), errors::kNotFound, "file not open");
    // ...
}
```

---

## 6. 模块自定义错误

业务模块的错误码应从 **10000** 开始，建议每个模块分配 100 个号段（如数据库模块 10000~10099、日志模块 10100~10199）。

### 6.1 X-Macro 模式（推荐）

使用 X-Macro 模式只需三步，自动生成错误码常量 + 元数据表 + 查表函数。

**第一步：定义错误列表**

```cpp
#define DB_ERRORS(X) \
    X(10001, ConnectFail,     "Database connection failed",              ef::Severity::Error) \
    X(10002, QueryTimeout,    "Query execution timeout",                 ef::Severity::Error) \
    X(10003, DupKey,          "Duplicate key violation in insert",       ef::Severity::Warn)  \
    X(10004, NotInitialized,  "Database not initialized",               ef::Severity::Fatal)
```

**第二步：生成错误码常量**

```cpp
namespace db::errors {
    EF_GEN_ERROR_CODES(DB_ERRORS)
}
// 展开后生成：
// constexpr int kConnectFail  = 10001;
// constexpr int kQueryTimeout = 10002;
// ...
```

**第三步：生成元数据表和查表函数**

```cpp
namespace db {
    EF_GEN_ERROR_TABLE(DB_ERRORS)
}
// 展开后生成：
// inline constexpr ef::ErrorInfo kErrorTable[] = { ... };
// inline const ef::ErrorInfo* LookupError(int code) noexcept { ... }

#undef DB_ERRORS
```

**第四步：注册外部查表（在初始化代码中调用一次）**

```cpp
ef::SetExternalLookup(db::LookupError);
```

### 6.2 手动定义模式

最简单的方式，适用于不需要错误元数据（名称、描述、严重程度）的场景：

```cpp
namespace db::errors {
    constexpr int kConnectFail  = 10001;
    constexpr int kQueryTimeout = 10002;
}

// 使用时：EF_ERROR(db::errors::kConnectFail, "server %s", addr);
```

此时框架将无法识别模块错误码的名称，`what()` 会回退到 `"Unknown"`，`severity()` 回退到 `Error`。

---

## 7. 使用示例

### 7.1 基础用法

```cpp
#include "ef/ef.h"
using namespace ef::errors;

// 返回 Result<T>
ef::Result<std::string> ReadFile(const std::string& path) {
    if (path.empty()) {
        return EF_ERROR(kInvalidArg, "path must not be empty");
    }
    if (path == "notfound.cfg") {
        return EF_ERROR(kNotFound, "cannot open '%s'", path.c_str());
    }
    return std::string("file content...");
}

// 使用 Result
auto r = ReadFile("config.ini");
if (!r) {
    // 处理错误
    ef::LogError(r.unwrap_err());
} else {
    // 使用成功值
    std::printf("content: %s\n", r->c_str());
}
```

### 7.2 错误链

```cpp
ef::Result<std::string> FetchData(const std::string& host, int port) {
    // 底层错误
    auto low = EF_ERROR(kConnectionRefused, "connect to %s:%d failed", host.c_str(), port);
    // 上层错误，附加上下层原因
    auto high = EF_ERROR(kTimeout, "retry exhausted after 3 tries")
                    .caused_by(std::move(low));
    return high;
}

// 输出错误链
auto r = FetchData("10.0.0.1", 8080);
if (!r) {
    ef::LogError(r.unwrap_err());
}
// 输出：
// [Error] Timeout (200) @ main.cpp:15 | retry exhausted after 3 tries
//   -> caused by: ConnectionRefused (201) @ main.cpp:12 | connect to 10.0.0.1:8080
```

### 7.3 value_or 默认值

```cpp
auto r = LoadConfig("notfound.cfg");
// 错误时使用默认端口 3000
ServerConfig cfg = std::move(r).value_or(ServerConfig{3000, false});
```

### 7.4 void Result 与 EF_CHECK

```cpp
ef::Result<void> ValidateConfig(const ServerConfig& cfg) {
    EF_RETURN_IF(cfg.port <= 0, kInvalidArg, "port must be positive");
    return {};
}

ef::Result<void> InitServer() {
    auto cfg = EF_TRY(LoadConfig("server.cfg"));
    EF_CHECK(ValidateConfig(cfg));  // 失败则提前返回
    return {};
}
```

### 7.5 模块错误集成

```cpp
#include "ef/ef.h"
#include "db_errors.h"

int main() {
    // 注册模块错误查表
    ef::SetExternalLookup(db::LookupError);

    // 使用模块错误码
    auto err = EF_ERROR(db::errors::kConnectFail, "cannot reach 10.0.0.1:5432");
    ef::LogError(err);
    // 输出：[Error] ConnectFail (10001) @ main.cpp:10 | cannot reach 10.0.0.1:5432
}
```

### 7.6 错误日志与终止

```cpp
void critical_path() {
    auto result = LoadConfig("server.cfg");
    if (!result) {
        // 仅输出日志，继续执行
        ef::LogError(result.unwrap_err(), "WARNING: ");

        // 或：致命错误，输出日志并终止
        ef::Panic(result.unwrap_err());
    }
}
```

---

## 8. 配置与集成

### 编译器要求

- C++17 或更高版本
- 支持的编译器：GCC 7+、Clang 5+、MSVC 2017+

### 集成步骤

1. 将 `error-framework/include/` 目录添加到项目的头文件搜索路径

2. 在源文件中包含汇总头文件：

```cpp
#include "ef/ef.h"
```

3. 引入错误码命名空间（可选，推荐）：

```cpp
using namespace ef::errors;
```

### 编译选项

无特殊编译要求。框架为 header-only，不需要链接额外库。

**CMake 集成示例：**

```cmake
target_include_directories(your_target PRIVATE
    ${CMAKE_SOURCE_DIR}/path/to/error-framework/include
)
```

---

## 9. 内存模型

### Error 对象大小

| 字段 | 大小（64位系统） |
|------|------------------|
| `code_` (int) | 4 字节 |
| `location_` (SourceLocation) | 24 字节（3 × 8 字节指针） |
| `sbo_` (char[64]) | 64 字节 |
| `heap_` (std::string) | 32 字节（典型实现） |
| `cause_` (unique_ptr) | 8 字节 |
| **总计** | ~132 字节 |

### 分配行为

| 场景 | 堆分配次数 |
|------|-----------|
| `EF_ERROR(code, "short msg")` — 消息 ≤ 63 字节 | 0 |
| `EF_ERROR(code, "very long message...")` — 消息 > 63 字节 | 1（`std::string`） |
| `err.caused_by(std::move(sub_err))` | 1（`unique_ptr<Error>`） |
| `Error` 构造为 `Result` 的错误值 | 0（placement new 在 Result 内部 buf） |

> 常见路径（参数校验失败、简单的 "not found" 等）通常零堆分配。

---

## 10. API 参考

### Error 类

| 方法 | 返回类型 | 说明 |
|------|---------|------|
| `Error(code, loc, fmt, args...)` | — | 构造函数，printf 风格格式化 |
| `Error(code, loc)` | — | 构造函数，仅错误码 |
| `Error(const Error&)` | — | 深拷贝构造 |
| `caused_by(Error&&)` | `Error&` | 附加错误链，链式调用 |
| `code()` | `int` | 获取错误码 |
| `location()` | `SourceLocation` | 获取源码位置 |
| `severity()` | `Severity` | 获取严重程度 |
| `what()` | `const char*` | 错误名称（来自注册表） |
| `description()` | `const char*` | 错误描述（来自注册表） |
| `message()` | `std::string` | 上下文消息（动态信息） |
| `has_message()` | `bool` | 是否有上下文消息 |
| `cause()` | `const Error*` | 错误链下层原因指针 |
| `has_cause()` | `bool` | 是否有错误链 |
| `is_ok()` | `bool` | code == 0 |
| `to_string()` | `std::string` | 完整格式化输出（含错误链） |

### Result<T, E> 类

| 方法 | 返回类型 | 说明 |
|------|---------|------|
| `Result(T&&)` | — | 从成功值构造（移动） |
| `Result(E&&)` | — | 从错误构造（移动） |
| `is_ok()` | `bool` | 是否持有成功值 |
| `is_err()` | `bool` | 是否持有错误 |
| `operator bool()` | `bool` (explicit) | if (r) 判断是否成功 |
| `unwrap()` | `const T&` / `T&` | 解包成功值（错误时 abort） |
| `unwrap_move()` | `T` | 移动解包成功值 |
| `unwrap_err()` | `const E&` / `E&` | 解包错误值 |
| `value_or(T&&)` | `T` | 移动版本：错误时返回默认值 |
| `value_or(const T&)` | `T` | 拷贝版本：错误时返回默认值 |
| `operator*()` | `T&` / `const T&` | 解引用成功值 |
| `operator->()` | `T*` / `const T*` | 指针访问成员 |

### Result<void, E> 类

| 方法 | 返回类型 | 说明 |
|------|---------|------|
| `Result()` | — | 默认构造，成功状态 |
| `Result(E&&)` | — | 从错误构造 |
| `is_ok()` | `bool` | 是否成功 |
| `is_err()` | `bool` | 是否错误 |
| `operator bool()` | `bool` (explicit) | 状态判断 |
| `unwrap_err()` | `const E&` / `E&` | 解包错误值 |

### 宏参考

| 宏 | 说明 |
|----|------|
| `EF_SOURCE_LOC()` | 捕获当前源码位置 |
| `EF_ERROR(code, fmt, ...)` | 构造 Error，自动捕获源码位置 |
| `EF_MAKE(code, fmt, ...)` | `EF_ERROR` 的别名 |
| `EF_TRY(expr)` | 解包 Result，错误时自动传播 |
| `EF_CHECK(expr)` | 检查 Result\<void\>，失败时提前返回 |
| `EF_RETURN_IF(cond, code, fmt, ...)` | 条件为真时返回错误 |
| `EF_EXPECT_OR(cond, code, fmt, ...)` | 条件为假时返回错误 |

### 模块错误定义宏

| 宏 | 说明 |
|----|------|
| `EF_GEN_ERROR_CODES(XMacroList)` | 从 X-Macro 列表生成 `constexpr int` 错误码常量 |
| `EF_GEN_ERROR_TABLE(XMacroList)` | 从 X-Macro 列表生成 `ErrorInfo[]` 元数据表 + 查表函数 |

### 工具函数

| 函数 | 说明 |
|------|------|
| `void LogError(const Error&, const char* prefix = "")` | 输出错误到 stderr，含错误链 |
| `std::string ErrorString(const Error&)` | 返回格式化错误字符串 |
| `void Panic(const Error&)` | 输出日志后 std::abort() |
| `void SetExternalLookup(ExternalLookupFn)` | 注册外部错误查表回调 |
| `const ErrorInfo* LookupError(int)` | 查错误元数据（内置+外部） |

---

## 11. 常见问题解答

### Q1: 如何区分 `what()`、`description()` 和 `message()`？

- `what()` — 返回注册表中错误码对应的**名称**，如 `"NotFound"`。这是静态字符串。
- `description()` — 返回注册表中错误码对应的**描述**，如 `"Resource not found"`。这也是静态字符串。
- `message()` — 返回 Error 构造时传入的**动态上下文消息**，如 `"cannot open 'config.ini'"`。

### Q2: 为什么 Result 禁止拷贝？

`Result` 是 tagged union，同一块内存可能存储 T 或 E。如果允许拷贝，调用方可能无意中复制了错误值导致双重释放或逻辑混淆。需要传递时请使用 `std::move`。

### Q3: `EF_TRY` 和 `EF_CHECK` 有什么区别？

- `EF_TRY(expr)`：用于需要**取值**的场景。expr 返回 `Result<T>`，成功时返回解包后的 `T` 值；失败时自动 return 错误。
- `EF_CHECK(expr)`：用于仅需**检查状态**的场景。expr 返回 `Result<void>`，成功则继续，失败则提前 return 错误。

### Q4: 如何在多个模块中组合外部查表？

因为 `SetExternalLookup` 是全局单例，多模块需要组合使用。推荐在应用入口层创建组合回调：

```cpp
ef::SetExternalLookup([](int code) -> const ef::ErrorInfo* {
    if (auto* info = db::LookupError(code)) return info;
    if (auto* info = cache::LookupError(code)) return info;
    if (auto* info = auth::LookupError(code)) return info;
    return nullptr;
});
```

### Q5: 错误码为 0 时是什么含义？

`kOk = 0` 表示成功状态，不对应任何错误。`Error::is_ok()` 就是通过 `code == 0` 判断的。框架内置表中该条目严重程度为 `Debug`。

### Q6: 为什么不使用 C++ 异常？

异常在大型系统中存在若干问题：性能不可预测（栈展开成本）、控制流不透明（难以审计哪些路径会抛异常）、ABI 不兼容问题。`Result<T, E>` 让错误路径在函数签名中显式可见，编译期即可检查。

### Q7: SBO 的 64 字节限制够用吗？

64 字节足以容纳绝大多数错误上下文（如 `"cannot open 'config.ini'"` 仅 27 字节）。超出时自动切换到堆存储，对调用方完全透明。可根据需要修改 `Error::kSboSize` 常量调整阈值。

### Q8: 模块错误码注册后未立即生效？

确保在**任何错误构造之前**调用 `SetExternalLookup`。推荐的调用时机是 `main()` 函数开头（在 `DbInit()` 等函数调用之前）。

---

## 12. 扩展指南

### 添加新的内置错误码

在 `ef/error_kind.h` 中进行三步操作：

**1. 在 `ef::errors` 命名空间中添加常量：**

```cpp
constexpr int kDiskFull = 104;  // IO 类，号段 100~199
```

**2. 在 `kBuiltinErrorTable[]` 中添加元数据条目：**

```cpp
{ errors::kDiskFull, "DiskFull", "No space left on device", Severity::Error },
```

### 添加新的工具函数

在 `ef/utils.h` 中添加自定义工具函数。例如，实现 JSON 格式的错误输出：

```cpp
#include "ef/utils.h"
#include <sstream>

namespace ef {
inline std::string ErrorToJson(const Error& err) {
    std::ostringstream os;
    os << R"({"code":)" << err.code()
       << R"(,"what":")" << err.what()
       << R"(","location":")" << err.location().file << ":" << err.location().line
       << R"(","message":")" << (err.has_message() ? err.message() : "")
       << R"("})";
    return os.str();
}
}  // namespace ef
```

### 添加新的便捷宏

在 `ef/macros.h` 中添加自定义宏。例如，实现带自定义错误转换的 TRY：

```cpp
#define EF_TRY_AS(expr, err_code, fmt, ...)               \
    ({                                                     \
        auto _ef_try_as_result = (expr);                   \
        if (!_ef_try_as_result) {                          \
            return EF_ERROR(err_code, fmt, ##__VA_ARGS__); \
        }                                                  \
        _ef_try_as_result.unwrap_move();                   \
    })
```

### 支持自定义错误类型 E

`Result<T, E>` 的默认 E 是 `Error`，但可以替换为自定义错误类型。自定义 E 需要满足：

- 支持拷贝和移动语义
- 与 `EF_TRY` 等宏兼容（宏中 `unwrap_err()` 的返回类型需与函数返回的错误类型匹配）

```cpp
struct MyError {
    int code;
    std::string desc;
};

// 使用自定义错误类型
Result<int, MyError> DoSomething() {
    if (should_fail) return MyError{500, "internal error"};
    return 42;
}
```

### 错误号段管理建议

| 号段 | 模块 |
|------|------|
| `10000 ~ 10099` | 数据库模块 |
| `10100 ~ 10199` | 缓存模块 |
| `10200 ~ 10299` | 认证模块 |
| `10300 ~ 10399` | 配置模块 |
| `10400 ~ 10499` | 网络通信模块 |
| ... | ... |

建议在项目文档中维护号段分配表，避免冲突。

---

> **许可协议：** MIT License
>
> **作者：** backend-common-components
>
> **日期：** 2026-06-03