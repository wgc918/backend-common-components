# EF Error Handling Framework

一个轻量级、高性能的 C++ 错误处理框架，提供类型安全的 `Result<T, E>` 机制、可级联的错误对象、源码追踪以及可扩展的错误码体系。

---

## 目录

- [1. 设计理念](#1-设计理念)
- [2. 架构概览](#2-架构概览)
- [3. 核心实现原理](#3-核心实现原理)
- [4. 模块详解](#4-模块详解)
  - [4.1 utils.h — 源码定位](#41-utilsh--源码定位)
  - [4.2 error_kind.h — 错误码体系](#42-error_kindh--错误码体系)
  - [4.3 error.h / error.cpp — 错误对象](#43-errorh--errorcpp--错误对象)
  - [4.4 result.h — Result 类型](#44-resulth--Result-类型)
  - [4.5 macros.h — 便捷宏](#45-macrosh--便捷宏)
  - [4.6 error_def.h — 自定义错误生成](#46-error_defh--自定义错误生成)
  - [4.7 ef.h — 汇总头文件](#47-efh--汇总头文件)
- [5. 模块交互逻辑](#5-模块交互逻辑)
- [6. 快速开始](#6-快速开始)
  - [6.1 安装与集成](#61-安装与集成)
  - [6.2 基础用法](#62-基础用法)
- [7. 使用指南](#7-使用指南)
  - [7.1 创建错误](#71-创建错误)
  - [7.2 错误级联](#72-错误级联)
  - [7.3 Result 类型的使用](#73-result-类型的使用)
  - [7.4 错误传播宏](#74-错误传播宏)
  - [7.5 自定义错误码](#75-自定义错误码)
  - [7.6 外部错误注册](#76-外部错误注册)
- [8. 完整示例](#8-完整示例)
- [9. 最佳实践](#9-最佳实践)
- [10. API 参考](#10-api-参考)

---

## 1. 设计理念

本框架的设计围绕以下核心原则：

### 1.1 显式错误传播

传统 C++ 异常机制存在隐式控制流、性能开销大、难以追踪传播路径等问题。本框架采用 **返回值式错误处理**，通过 `Result<T, E>` 类型强制调用方显式处理错误，使错误传播路径清晰可追溯。

### 1.2 零开销抽象

- **小对象优化 (SBO)**：Error 对象的上下文消息在 ≤ 64 字节时完全存储在栈上，避免堆分配。
- **placement new**：Result 使用 tagged union + placement new 实现，无虚函数、无额外间接层。
- **编译期查表**：内置错误表为 `constexpr`，查找在编译期即可优化为常量。

### 1.3 可扩展的错误码体系

框架提供 10 个内置通用错误码，同时通过 X-Macro 机制和外部注册机制支持业务模块定义自己的错误码，无需修改框架源码。

### 1.4 源码级可追溯

每个 Error 对象自动携带 `__FILE__`、`__LINE__`、`__FUNCTION__` 信息，便于定位问题根因。

---

## 2. 架构概览

```
┌─────────────────────────────────────────────────┐
│                    ef.h                          │
│              (汇总头文件，一键引入)                │
├─────────────────────────────────────────────────┤
│  ┌───────────┐  ┌──────────────┐  ┌───────────┐ │
│  │  utils.h  │  │ error_kind.h │  │ result.h  │ │
│  │           │  │              │  │           │ │
│  │ Source-   │  │ ErrorLevel   │  │ Result    │ │
│  │ Location  │  │ ErrorInfo    │  │ <T,E>     │ │
│  │           │  │ 内置错误码    │  │ Result    │ │
│  └─────┬─────┘  │ Lookup函数   │  │ <void,E>  │ │
│        │        └──────┬───────┘  └─────┬─────┘ │
│        │               │               │       │
│  ┌─────┴─────┐  ┌──────┴───────┐       │       │
│  │ macros.h  │  │  error.h     │       │       │
│  │           │  │              │       │       │
│  │ EF_ERROR  │  │ Error 类     │◄──────┘       │
│  │ EF_MAKE   │  │ - SBO        │               │
│  │ EF_TRY    │  │ - 级联原因    │               │
│  │ EF_CHECK  │  │ - 外部注册    │               │
│  └───────────┘  └──────┬───────┘               │
│                        │                       │
│                 ┌──────┴───────┐               │
│                 │ error_def.h  │               │
│                 │              │               │
│                 │ X-Macro 宏   │               │
│                 │ 代码生成      │               │
│                 └──────────────┘               │
└─────────────────────────────────────────────────┘
```

---

## 3. 核心实现原理

### 3.1 Tagged Union (Result)

`Result<T, E>` 是一个 tagged union，核心实现：

- 使用 `bool m_ok` 标记当前持有的是成功值 `T` 还是错误值 `E`。
- 使用 `unsigned char m_buf[max(sizeof(T), sizeof(E))]` 作为原始存储，通过 **placement new** 在其中构造 `T` 或 `E`。
- 拷贝构造被删除，只允许移动语义，避免大对象的意外拷贝。
- 析构时根据 `m_ok` 标记调用对应类型的析构函数。

```
Result 内存布局:
┌──────┬────────────────────────────────────────┐
│ m_ok │  m_buf (大小 = max(sizeof(T), sizeof(E))) │
│ bool │  alignas(max(alignof(T), alignof(E)))    │
└──────┴────────────────────────────────────────┘
```

### 3.2 小对象优化 (SBO)

`Error` 对象内部维护一个 64 字节的栈缓冲区 `m_sbo_buf`：

- 格式化上下文消息时，先用 `snprintf` 尝试写入栈缓冲区。
- 若实际长度 < 64 字节 → 数据留在栈上，零堆分配。
- 若实际长度 ≥ 64 字节 → 分配堆内存 `m_heap_buf`，同时清空 `m_sbo_buf[0]`。
- `message()` 方法通过检查 `m_sbo_buf[0] == '\0'` 判断数据位置。

### 3.3 错误码查表机制

```
LookupError(code)
    │
    ├─► LookupInTable(kBuiltinErrorTable, code)  // 先查内置表 (constexpr)
    │       └─► 找到 → 返回 ErrorInfo*
    │
    ├─► LookupExternalError(code)                 // 再查外部注册函数
    │       └─► getExternalLookupFunc()(code)
    │               └─► 找到 → 返回 ErrorInfo*
    │
    └─► 返回 kUnknown 的 ErrorInfo*               // 兜底
```

查表优先级：**内置表 → 外部注册函数 → 兜底 Unknown**。外部查表函数通过 `setExternalLookup()` 注册，支持业务模块扩展。

---

## 4. 模块详解

### 4.1 utils.h — 源码定位

定义 `SourceLocation` 结构体，封装错误发生的源码位置信息：

```cpp
struct SourceLocation {
    const char* file;      // __FILE__
    int         line;      // __LINE__
    const char* function;  // __FUNCTION__
};
```

宏 `EF_SOURCE_LOC()` 便捷生成实例：

```cpp
#define EF_SOURCE_LOC() \
    ::ef::SourceLocation { __FILE__, __LINE__, __FUNCTION__ }
```

### 4.2 error_kind.h — 错误码体系

#### 错误等级枚举

| 等级 | 含义 | 适用场景 |
|------|------|----------|
| `Avoid` | 可避免的错误 | 调用方传入错误参数后可修正 |
| `CBC` (Case By Case) | 级联错误 | 由上游错误传播导致 |
| `Warning` | 警告 | 不影响主流程的异常情况 |
| `Fatal` | 严重错误 | 不可恢复，通常需要终止 |

#### 内置错误码 (ef::errors 命名空间)

| 常量 | 值 | 描述 | 等级 |
|------|-----|------|------|
| `kUnknown` | 1 | 未知错误 | Avoid |
| `kInvalidArg` | 2 | 无效参数 | Avoid |
| `kNotSupported` | 3 | 操作不支持 | Warning |
| `kNotFound` | 4 | 资源未找到 | Warning |
| `kAlreadyExists` | 5 | 资源已存在 | Warning |
| `kTimeout` | 6 | 操作超时 | Warning |
| `kInvalidFormat` | 7 | 无效格式 | Warning |
| `kOverflow` | 8 | 缓冲区溢出 | Fatal |
| `kOutOfRange` | 9 | 索引超出范围 | Fatal |
| `kEmpty` | 10 | 容器为空 | Warning |

#### 查表函数

- `LookupInTable(table, code)` — 在编译期数组中查找。
- `LookupError(code)` — 统一查找：先内置表 → 外部注册 → 兜底 Unknown。

### 4.3 error.h / error.cpp — 错误对象

`Error` 类是该框架的核心运行时错误载体：

```cpp
// 内存布局:
class Error {
    int                     m_code;               // 错误码
    SourceLocation          m_location;           // 源码位置
    char                    m_sbo_buf[64];        // SBO 栈缓冲区
    std::string             m_heap_buf;           // 堆缓冲区（溢出时使用）
    std::unique_ptr<Error>  m_cause;              // 级联原因（可选）
};
```

#### 关键方法

| 方法 | 说明 |
|------|------|
| `code()` | 返回错误码 |
| `name()` | 通过查表返回错误码名称 |
| `what()` | 通过查表返回错误码描述 |
| `level()` | 通过查表返回错误等级 |
| `message()` | 返回用户自定义的上下文消息 |
| `location()` | 返回源码位置 |
| `caused_by(Error&&)` | 设置级联原因，支持链式调用 |
| `has_cause()` | 是否存在级联原因 |
| `cause()` | 获取级联原因指针 |
| `to_string()` | 返回完整错误描述字符串（含级联链路） |

#### to_string() 输出格式示例

```
[Warning]NotFound file.cpp:42 user 'admin' not found in database
  caused by : [Fatal]Timeout connection.cpp:128 connection to 10.0.0.1:5432 timed out
```

### 4.4 result.h — Result 类型

#### Result<T, E>

泛型结果类型，同时只能持有成功值 `T` 或错误值 `E`：

```cpp
Result<User, Error> result = fetchUser(id);
if (result) {                         // 或 result.is_ok()
    User user = result.unwrap();      // 获取成功值，err时 abort
    auto name = result->name;         // 指针访问
} else {
    Error err = result.unwrap_err();  // 获取错误值
}
User user = result.value_or(defaultUser);  // 安全提取，提供默认值
```

#### Result<void, E>

无值特化版本，仅表示操作成功或失败：

```cpp
Result<void, Error> result = validateInput(data);
if (!result) {
    return result.unwrap_err();  // 传播错误
}
```

拷贝构造和拷贝赋值被删除，仅支持移动语义：

```cpp
Result<Data, Error> a = func();   // 移动构造
Result<Data, Error> b = std::move(a);  // 移动赋值
// Result<Data, Error> c = a;     // ❌ 编译错误
```

### 4.5 macros.h — 便捷宏

| 宏 | 说明 | 用法 |
|----|------|------|
| `EF_ERROR(code, fmt, ...)` | 创建带上下文消息的错误 | `EF_ERROR(kNotFound, "user %s not found", name)` |
| `EF_MAKE(code)` | 创建无上下文消息的错误 | `EF_MAKE(kTimeout)` |
| `EF_TRY(expr)` | 传播错误：Err 时返回，Ok 时解包值 | 见下方 |
| `EF_CHECK(expr)` | 检查错误：Err 时返回（不返回值） | `EF_CHECK(validate());` |

#### EF_TRY 展开逻辑

```cpp
// 原始写法:
auto result = fetchUser(id);
if (!result)
    return result.unwrap_err();
User user = result.unwrap_move();

// 等价于:
User user = EF_TRY(fetchUser(id));
```

#### EF_CHECK 展开逻辑

```cpp
// 原始写法:
auto result = validateInput(data);
if (!result)
    return result.unwrap_err();

// 等价于:
EF_CHECK(validateInput(data));
```

### 4.6 error_def.h — 自定义错误生成

通过 X-Macro 机制，业务模块可零侵入地定义自己的错误码和查表函数。

#### X-Macro 列表格式

```cpp
// 每项: (错误码值, 错误名, 描述, 错误等级)
#define DB_ERRORS(X)           \
    X(1001, ConnectionFailed,  "Database connection failed",            ::ef::ErrorLevel::Fatal)   \
    X(1002, QueryTimeout,      "Database query timed out",             ::ef::ErrorLevel::Warning) \
    X(1003, DuplicateKey,      "Duplicate primary key violation",      ::ef::ErrorLevel::Avoid)   \
    X(1004, PermissionDenied,  "Insufficient database permissions",    ::ef::ErrorLevel::Fatal)
```

#### 生成的代码

```cpp
// EF_GEN_ERROR_CODES(DB_ERRORS) 展开为:
constexpr int kConnectionFailed = 1001;
constexpr int kQueryTimeout     = 1002;
constexpr int kDuplicateKey     = 1003;
constexpr int kPermissionDenied = 1004;

// EF_GEN_ERROR_META(DB_ERRORS) 展开为:
inline constexpr ::ef::ErrorInfo kErrorTable[] = {
    {1001, "ConnectionFailed", "Database connection failed",          ::ef::ErrorLevel::Fatal},
    {1002, "QueryTimeout",     "Database query timed out",            ::ef::ErrorLevel::Warning},
    {1003, "DuplicateKey",     "Duplicate primary key violation",     ::ef::ErrorLevel::Avoid},
    {1004, "PermissionDenied", "Insufficient database permissions",   ::ef::ErrorLevel::Fatal},
};
inline const ::ef::ErrorInfo* LookopError(int code) noexcept {
    return ::ef::LookupInTable(KErrorTable, code);
}
```

### 4.7 ef.h — 汇总头文件

业务模块只需一行引入：

```cpp
#include "ef/ef.h"
```

---

## 5. 模块交互逻辑

### 典型错误处理流程图

```
┌──────────────────────────────────────────────────────────────┐
│ 业务代码                                                      │
│                                                              │
│ Result<T, Error> func() {                                    │
│     auto val = EF_TRY(anotherFunc());  ◄── macros.h (EF_TRY) │
│     if (invalid(val))                                        │
│         return EF_ERROR(kInvalidArg, "reason: %d", reason);  │
│                              ◄── macros.h (EF_ERROR)         │
│                              ◄── error.h (Error 构造)        │
│                              ◄── utils.h (EF_SOURCE_LOC)     │
│     return Result<T, Error>(val);  ◄── result.h              │
│ }                                                            │
│                                                              │
│ 调用方:                                                       │
│ auto result = func();                                        │
│ if (!result) {                                               │
│     log(result.unwrap_err().to_string());                    │
│                       ◄── error.h (to_string)                │
│                       ◄── error_kind.h (LookupError)         │
│ }                                                            │
└──────────────────────────────────────────────────────────────┘
```

### 自定义错误码注册流程

```
┌─────────────────┐     setExternalLookup()      ┌──────────────────┐
│   main() / 初始化 │ ──────────────────────────► │ error.cpp         │
│                  │                              │                  │
│  注册 DB 模块的    │                              │ getExternalLookup │
│  LookopError      │                              │ Func() = yourFunc│
└─────────────────┘                              └────────┬─────────┘
                                                          │
                                                          │ 运行时查询
                                                          ▼
                                                  ┌──────────────────┐
                                                  │ LookupError(code) │
                                                  │                  │
                                                  │ 1. 查内置表       │
                                                  │ 2. 查外部注册函数  │
                                                  │ 3. 兜底 Unknown   │
                                                  └──────────────────┘
```

---

## 6. 快速开始

### 6.1 安装与集成

本框架为 header-only 库（error.cpp 和 error_kind.cpp 需要编译），集成步骤如下：

**目录结构建议：**

```
your-project/
├── include/
│   └── ef/
│       ├── ef.h
│       ├── error.h
│       ├── error_def.h
│       ├── error_kind.h
│       ├── macros.h
│       ├── result.h
│       └── utils.h
├── src/
│   ├── error.cpp        # 框架源文件
│   ├── error_kind.cpp   # 框架源文件
│   └── ...
└── CMakeLists.txt
```

**CMakeLists.txt 示例：**

```cmake
cmake_minimum_required(VERSION 3.14)
project(your-project)

# 框架源文件
add_library(ef_error
    src/error.cpp
    src/error_kind.cpp
)

target_include_directories(ef_error PUBLIC include)

# 链接到你的目标
add_executable(my_app main.cpp)
target_link_libraries(my_app ef_error)
```

**要求：**
- C++17 或更高版本
- 标准库：`<cstdio>`, `<cstring>`, `<memory>`, `<string>`, `<vector>`, `<new>`, `<type_traits>`, `<utility>`

### 6.2 基础用法

```cpp
#include "ef/ef.h"
#include <iostream>

// 定义返回 Result 的函数
ef::Result<int, ef::Error> divide(int a, int b) {
    if (b == 0) {
        return EF_ERROR(ef::errors::kInvalidArg, "division by zero, a=%d", a);
    }
    return ef::Result<int, ef::Error>(a / b);
}

int main() {
    auto result = divide(10, 0);
    if (!result) {
        std::cerr << result.unwrap_err().to_string() << std::endl;
        return 1;
    }
    std::cout << "result: " << *result << std::endl;
    return 0;
}
```

输出：

```
[Avoid]InvalidArg main.cpp:7 division by zero, a=10
```

---

## 7. 使用指南

### 7.1 创建错误

```cpp
// 带上下文消息（推荐）
return EF_ERROR(ef::errors::kNotFound, "user '%s' not found in table '%s'", username, table);

// 不带上下文消息（错误描述从查表中获取）
return EF_MAKE(ef::errors::kTimeout);

// 手动构造（不推荐，宏更简洁）
return ef::Error(ef::errors::kInvalidArg, EF_SOURCE_LOC(), "x must be positive, got %d", x);
```

### 7.2 错误级联

当底层函数返回错误时，可以将其作为当前错误的"原因"进行级联，保留完整的错误链路：

```cpp
ef::Result<User, ef::Error> getUser(int id) {
    auto dbResult = dbQuery("SELECT * FROM users WHERE id = ?", id);
    if (!dbResult) {
        // 将底层 DB 错误作为原因进行包装
        return EF_ERROR(ef::errors::kNotFound, "failed to fetch user id=%d", id)
            .caused_by(std::move(dbResult.unwrap_err()));
    }
    return ef::Result<User, ef::Error>(User{dbResult.unwrap()});
}
```

级联错误通过 `to_string()` 输出时会递归显示完整链路：

```
[Warning]NotFound main.cpp:15 failed to fetch user id=42
  caused by : [Fatal]Timeout connection.cpp:128 connection to 10.0.0.1:5432 timed out
```

### 7.3 Result 类型的使用

#### Result<T, E> — 有返回值

```cpp
ef::Result<std::string, ef::Error> readFile(const char* path) {
    // ... 读取文件逻辑 ...
    if (file_not_found) {
        return EF_ERROR(ef::errors::kNotFound, "file '%s' does not exist", path);
    }
    return ef::Result<std::string, ef::Error>(content);
}

// 使用
auto result = readFile("/etc/config.json");
if (result) {
    std::string content = result.unwrap_move();
    process(content);
}

// 安全提取，提供默认值
std::string content = result.value_or("{}");
```

#### Result<void, E> — 无返回值

```cpp
ef::Result<void, ef::Error> validateEmail(const std::string& email) {
    if (email.find('@') == std::string::npos) {
        return EF_ERROR(ef::errors::kInvalidFormat, "invalid email: %s", email.c_str());
    }
    return ef::Result<void, ef::Error>::OK();
}

// 使用
EF_CHECK(validateEmail(userEmail));
// 到达此处表示验证通过
```

### 7.4 错误传播宏

```cpp
// EF_TRY: 提取成功值，遇到错误立即返回
ef::Result<Response, ef::Error> handleRequest(const Request& req) {
    auto user  = EF_TRY(authenticate(req.token));    // 失败则传播
    auto data  = EF_TRY(fetchData(user.id));          // 失败则传播
    auto resp  = EF_TRY(processData(data));           // 失败则传播
    return ef::Result<Response, ef::Error>(resp);
}

// EF_CHECK: 检查无返回值的操作，遇到错误立即返回
ef::Result<void, ef::Error> processOrder(Order& order) {
    EF_CHECK(validateOrder(order));                   // 失败则传播
    EF_CHECK(checkInventory(order));                  // 失败则传播
    EF_CHECK(chargePayment(order));                   // 失败则传播
    return ef::Result<void, ef::Error>::OK();
}
```

#### 宏使用注意事项

- `EF_TRY` 要求表达式的返回类型为 `Result<T, E>`，且当前函数的返回类型必须兼容。
- `EF_CHECK` 用于返回 `Result<void, E>` 的场景，不需要提取值。
- 两个宏都在检查到错误时执行 `return result.unwrap_err()`，确保函数签名返回 `Result<T, Error>`（其中 Error 为 `ef::Error`）。

### 7.5 自定义错误码

通过 X-Macro 定义业务模块错误码，零侵入框架代码：

**步骤 1：定义错误列表（单独头文件，如 `db_errors.h`）**

```cpp
#pragma once
#include "ef/error_def.h"

// X-Macro 错误列表
#define DB_ERRORS(X)                                                              \
    X(1001, ConnectionFailed,  "Database connection failed",              ::ef::ErrorLevel::Fatal)   \
    X(1002, QueryTimeout,      "Database query timed out",                ::ef::ErrorLevel::Warning) \
    X(1003, DuplicateKey,      "Duplicate primary key violation",         ::ef::ErrorLevel::Avoid)   \
    X(1004, PermissionDenied,  "Insufficient database permissions",       ::ef::ErrorLevel::Fatal)   \
    X(1005, TableNotFound,     "Specified table does not exist",          ::ef::ErrorLevel::Warning) \
    X(1006, DeadlockDetected,  "Transaction deadlock detected, retry",    ::ef::ErrorLevel::Avoid)

// 生成错误码常量
EF_GEN_ERROR_CODES(DB_ERRORS)

// 生成查表函数
EF_GEN_ERROR_META(DB_ERRORS)
```

**步骤 2：注册外部错误表（在初始化阶段，如 `main()` 中）**

```cpp
#include "db_errors.h"

int main() {
    // 注册 DB 模块的错误查表函数
    ef::setExternalLookup(LookopError);

    // ... 业务逻辑 ...
    return 0;
}
```

**步骤 3：使用自定义错误码**

```cpp
#include "db_errors.h"
#include "ef/ef.h"

ef::Result<QueryResult, ef::Error> executeQuery(const std::string& sql) {
    if (connection_lost) {
        return EF_ERROR(kConnectionFailed, "lost connection to '%s'", host);
    }
    // ...
}
```

#### X-Macro 自定义错误码的最佳实践

1. **错误码值范围**：建议不同模块使用不同的错误码范围，避免冲突（如 DB 模块 1000-1999，网络模块 2000-2999）。
2. **头文件组织**：每个模块定义自己的错误头文件，通过 `setExternalLookup()` 注册。
3. **多模块注册**：框架目前支持单一外部查表函数。如需支持多模块，可在注册函数内部进行分发。

### 7.6 外部错误注册

对于不使用 X-Macro 的场景，也可以手动实现查表函数并注册：

```cpp
// 自定义查表函数
const ef::ErrorInfo* myLookup(int code) noexcept {
    static const ef::ErrorInfo table[] = {
        {2001, "NetworkTimeout", "Network request timed out", ef::ErrorLevel::Warning},
        {2002, "DNSResolveFailed", "DNS resolution failed", ef::ErrorLevel::Fatal},
    };
    for (auto& entry : table) {
        if (entry.code == code) return &entry;
    }
    return nullptr;  // 未找到返回 nullptr，框架会兜底到 kUnknown
}

int main() {
    ef::setExternalLookup(myLookup);
    // ...
}
```

---

## 8. 完整示例

以下是一个完整的使用示例，展示错误码定义、函数链式调用和错误处理：

```cpp
// ──── db_errors.h ────
#pragma once
#include "ef/error_def.h"

#define DB_ERRORS(X)                                                              \
    X(1001, ConnectionFailed,  "Database connection failed",              ::ef::ErrorLevel::Fatal)   \
    X(1002, QueryTimeout,      "Database query timed out",                ::ef::ErrorLevel::Warning) \
    X(1003, DuplicateKey,      "Duplicate primary key violation",         ::ef::ErrorLevel::Avoid)

EF_GEN_ERROR_CODES(DB_ERRORS)
EF_GEN_ERROR_META(DB_ERRORS)

// ──── main.cpp ────
#include "db_errors.h"
#include "ef/ef.h"
#include <iostream>

// 模拟数据库操作
ef::Result<std::string, ef::Error> dbConnect(const std::string& host) {
    if (host.empty()) {
        return EF_ERROR(kConnectionFailed, "host is empty");
    }
    return ef::Result<std::string, ef::Error>("connected_to_" + host);
}

ef::Result<std::string, ef::Error> dbQuery(const std::string& conn, const std::string& sql) {
    if (sql.find("DROP") != std::string::npos) {
        return EF_ERROR(ef::errors::kNotSupported, "DROP is not allowed");
    }
    return ef::Result<std::string, ef::Error>("query_result_for: " + sql);
}

// 业务函数：链式调用数据库操作
ef::Result<std::string, ef::Error> fetchUserData(const std::string& userId) {
    auto conn = EF_TRY(dbConnect("localhost:5432"));
    auto data = EF_TRY(dbQuery(conn, "SELECT * FROM users WHERE id=" + userId));

    if (data.find("empty") != std::string::npos) {
        return EF_ERROR(ef::errors::kNotFound, "user %s not found", userId.c_str());
    }

    return ef::Result<std::string, ef::Error>(data);
}

int main() {
    // 注册自定义错误码
    ef::setExternalLookup(LookopError);

    auto result = fetchUserData("42");

    if (!result) {
        const auto& err = result.unwrap_err();
        std::cerr << "Error: " << err.to_string() << std::endl;
        std::cerr << "  code:  " << err.code() << std::endl;
        std::cerr << "  name:  " << err.name() << std::endl;
        std::cerr << "  level: " << static_cast<int>(err.level()) << std::endl;
        std::cerr << "  file:  " << err.location().file << ":" << err.location().line << std::endl;
        std::cerr << "  func:  " << err.location().function << std::endl;
        return 1;
    }

    std::cout << "Success: " << *result << std::endl;
    return 0;
}
```

---

## 9. 最佳实践

### 9.1 始终使用 Result 而非裸 Error

```cpp
// ❌ 不推荐：丢失类型信息
Error doSomething() { ... }

// ✅ 推荐：类型安全
Result<Data, Error> doSomething() { ... }
Result<void, Error> doSomething() { ... }
```

### 9.2 优先使用宏简化代码

```cpp
// ❌ 冗长
auto result = someFunc();
if (!result) return result.unwrap_err();
auto val = result.unwrap_move();

// ✅ 简洁
auto val = EF_TRY(someFunc());
```

### 9.3 为每个错误提供有意义的上下文消息

```cpp
// ❌ 信息不足
return EF_MAKE(ef::errors::kInvalidArg);

// ✅ 包含关键上下文
return EF_ERROR(ef::errors::kInvalidArg, "port must be in [1, 65535], got %d", port);
```

### 9.4 合理使用错误级联

只在需要保留底层错误信息时才使用 `caused_by`：

```cpp
// 当底层错误对上层有意义时
return EF_ERROR(kConnectionFailed, "failed to connect to %s", host)
    .caused_by(std::move(innerError));

// 当只是简单包装时，直接描述即可
return EF_ERROR(kConnectionFailed, "DNS resolve failed for %s", host);
```

### 9.5 错误码范围划分

| 模块 | 建议范围 | 说明 |
|------|----------|------|
| 框架内置 | 1 ~ 99 | 通用错误码 |
| 数据库模块 | 1000 ~ 1999 | 数据库相关 |
| 网络模块 | 2000 ~ 2999 | 网络通信 |
| 认证模块 | 3000 ~ 3999 | 鉴权认证 |
| 业务模块 | 10000+ | 具体业务逻辑 |

### 9.6 避免在析构函数中创建 Error

Error 对象在 Err 状态调用 `unwrap()` 或 `operator*()` 时会调用 `std::abort()`。析构函数中不应抛出或触发 abort。

### 9.7 Result 的移动语义

Result 禁止拷贝，只允许移动：

```cpp
Result<Data, Error> getData() {
    Data d = compute();
    return Result<Data, Error>(std::move(d));  // 移动构造
}

auto r1 = getData();       // 移动构造
auto r2 = std::move(r1);   // 移动赋值
```

---

## 10. API 参考

### Error 类

| 方法 | 返回类型 | 说明 |
|------|----------|------|
| `Error(int code, SourceLocation loc, const char* fmt, Args&&... args)` | 构造函数 | 创建带格式化上下文的错误 |
| `Error(int code, SourceLocation loc)` | 构造函数 | 创建无上下文的错误 |
| `code()` | `int` | 错误码 |
| `name()` | `const char*` | 错误码名称 |
| `what()` | `const char*` | 错误码描述 |
| `level()` | `ErrorLevel` | 错误等级 |
| `message()` | `std::string` | 用户上下文消息 |
| `location()` | `SourceLocation` | 源码位置 |
| `caused_by(Error&&)` | `Error&` | 设置级联原因（链式调用） |
| `has_cause()` | `bool` | 是否有级联原因 |
| `cause()` | `const Error*` | 级联原因指针 |
| `to_string()` | `std::string` | 完整错误描述（含级联） |

### Result<T, E>

| 方法 | 返回类型 | 说明 |
|------|----------|------|
| `is_ok()` | `bool` | 是否为成功状态 |
| `is_err()` | `bool` | 是否为错误状态 |
| `operator bool()` | `bool` | 隐式 bool 转换 |
| `unwrap()` | `T&` / `const T&` | 提取成功值（Err 时 abort） |
| `unwrap_move()` | `T` | 移动提取成功值（Err 时 abort） |
| `unwrap_err()` | `E&` / `const E&` | 提取错误值 |
| `value_or(T&&)` | `T` | 提取成功值或返回默认值 |
| `operator->()` | `T*` / `const T*` | 指针访问（Err 时 abort） |
| `operator*()` | `T&` / `const T&` | 解引用访问（Err 时 abort） |

### Result<void, E>

| 方法 | 返回类型 | 说明 |
|------|----------|------|
| `Result()` | 构造函数 | 默认构造为成功状态 |
| `OK()` | `Result<void, E>` | 工厂方法创建成功状态 |
| `is_ok()` | `bool` | 是否为成功状态 |
| `is_err()` | `bool` | 是否为错误状态 |
| `operator bool()` | `bool` | 隐式 bool 转换 |
| `unwrap_err()` | `E&` / `const E&` | 提取错误值 |

### 宏

| 宏 | 说明 |
|----|------|
| `EF_SOURCE_LOC()` | 生成 `SourceLocation{__FILE__, __LINE__, __FUNCTION__}` |
| `EF_ERROR(code, fmt, ...)` | 创建带上下文消息的 Error |
| `EF_MAKE(code)` | 创建无上下文消息的 Error |
| `EF_TRY(expr)` | 遇到 Err 时 `return err`，Ok 时提取值 |
| `EF_CHECK(expr)` | 遇到 Err 时 `return err`（无值提取） |
| `EF_GEN_ERROR_CODES(list)` | 从 X-Macro 列表生成错误码常量 |
| `EF_GEN_ERROR_META(list)` | 从 X-Macro 列表生成错误表和查表函数 |

### 自由函数

| 函数 | 说明 |
|------|------|
| `LookupError(code)` | 查内置表 → 查外部注册 → 兜底 Unknown |
| `LookupInTable(table, code)` | 在编译期数组中查找错误码 |
| `setExternalLookup(func)` | 注册外部查表函数 |
| `ErrorLevel2string(level)` | 错误等级转字符串 |