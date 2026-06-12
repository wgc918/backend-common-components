# INI 解析器

轻量级 C++17 INI 配置文件解析库，提供单例访问模式与隐式类型转换，支持标准 INI 格式的读取与查询。

## 设计原理

### 整体架构

```
┌──────────────┐      ┌───────────────────┐
│   IniFile     │──────│  Section          │
│  (单例)       │      │  (unordered_map)  │
│               │      │  key → Value      │
│  m_ini:       │      └───────────────────┘
│  section →    │
│  Section      │
└──────────────┘
```

- **IniFile**：全局单例，维护 `section名 → Section` 的映射。通过 `load()` 解析文件并填充内存结构。
- **Section**：`std::unordered_map<std::string, Value>`，表示一个配置节。
- **Value**：对字符串值的轻量封装，提供 `bool` / `int` / `double` / `std::string` 的隐式类型转换。

### 解析规则

| 规则 | 说明 |
|------|------|
| 节声明 | `[section_name]` 独占一行，方括号内为节名 |
| 键值对 | `key = value`，以第一个 `=` 分割，两边空白自动去除 |
| 注释 | 以 `#` 或 `;` 开头的行视为注释，会被忽略 |
| 空行 | 空白行直接跳过 |
| 全局键 | 出现在任何节声明之前的键值对自动归入 `Global` 节 |

### 单例模式

`IniFile` 采用 Meyers' Singleton 实现，通过静态局部变量保证线程安全（C++11 起）且延迟初始化。全局仅存在一个实例，`load()` 多次调用会清空旧数据并重新解析。

### Value 类型转换

`Value` 内部以 `std::string` 存储原始值，通过隐式转换运算符（`operator bool/int/double/std::string()`）实现按需类型转换。布尔转换仅在原始字符串为 `"true"` 时返回 `true`，其余均为 `false`。数值转换底层依赖 `std::atoi` / `std::atof`。

## 使用方式

### 编译集成

将 `include/` 加入头文件搜索路径，链接 `src/ini.cpp` 和 `src/value.cpp` 即可。

```bash
g++ -std=c++17 -I./include your_app.cpp src/ini.cpp src/value.cpp -o your_app
```

### 快速开始

```cpp
#include "ini.h"

int main() {
    IniFile& ini = IniFile::instance();
    ini.load("config.ini");

    // 访问节与键
    auto& db = ini["database"];
    std::string host = db["host"];       // 隐式转 string
    int port         = db["port"];       // 隐式转 int
    double timeout   = db["timeout"];    // 隐式转 double
    bool ssl         = db["use_ssl"];    // 隐式转 bool
    std::string raw  = db["host"].val(); // 获取原始字符串

    // 访问全局键
    std::string app = ini["Global"]["app_name"].val();
}
```

### 遍历所有配置

```cpp
IniFile& ini = IniFile::instance();
ini.show();  // 打印全部节与键值到 stdout
```

### 安全访问（带默认值）

```cpp
#include <stdexcept>

std::string get_str(const std::string& sec, const std::string& key,
                    const std::string& def = "") {
    try {
        auto& s = IniFile::instance()[sec];
        auto it = s.find(key);
        if (it != s.end()) return it->second.val();
    } catch (const std::out_of_range&) {}
    return def;
}

int get_int(const std::string& sec, const std::string& key, int def = 0) {
    try {
        auto& s = IniFile::instance()[sec];
        auto it = s.find(key);
        if (it != s.end()) return static_cast<int>(it->second);
    } catch (const std::out_of_range&) {}
    return def;
}
```

### 运行示例

```bash
cd examples
make all    # 编译 example 和 advanced_usage
make run    # 编译并运行所有示例
./example   # 基础用法：加载、类型转换、边界情况、错误处理
./advanced_usage  # 进阶用法：遍历、热加载、安全访问
```

## API 参考

### IniFile

| 方法 | 说明 |
|------|------|
| `static IniFile& instance()` | 获取单例引用 |
| `void load(const std::string& filename)` | 加载并解析 INI 文件，清空旧数据 |
| `Section& operator[](const std::string& section)` | 访问指定节，节不存在时抛出 `std::out_of_range` |
| `const Section& operator[](const std::string& section) const` | 只读访问 |
| `void show() const` | 打印全部内容到 stdout |
| `std::string trim(const std::string& str) const` | 去除首尾空白字符 |

### Value

| 方法 | 说明 |
|------|------|
| `operator bool() const` | 值为 `"true"` 时返回 `true` |
| `operator int() const` | 通过 `atoi` 转换 |
| `operator double() const` | 通过 `atof` 转换 |
| `operator std::string() const` | 返回原始字符串 |
| `std::string val() const` | 返回原始字符串（同 `operator string`） |

## 典型应用场景

- **服务配置管理**：数据库连接、Redis 缓存、日志级别等运行参数的集中管理
- **配置热加载**：配合文件变更监控，运行时动态重载配置而无需重启服务
- **多环境部署**：通过不同 `.ini` 文件区分开发、测试、生产环境
- **嵌入式配置**：适合无外部依赖、需要快速集成的轻量场景

## 注意事项

1. **单例约束**：`IniFile` 为全局单例，不可拷贝、不可移动。单例意味着同一进程内只能加载一份配置，如需同时管理多份配置，需自行扩展。
2. **不存在的节**：访问不存在的节会抛出 `std::out_of_range` 异常。访问不存在的键则返回空 `Value`（不会抛异常），调用者需自行判断。
3. **布尔语义**：`Value` 的布尔转换仅当原始字符串严格等于 `"true"` 时返回 `true`。`"1"`、`"yes"` 等常见布尔表示法会被视为 `false`。
4. **`load()` 覆盖**：每次调用 `load()` 会先清空已解析的全部数据，无法实现增量合并。如需合并多个文件，应在应用层自行处理。
5. **线程安全**：单例的静态局部变量初始化是线程安全的，但 `load()` 与数据访问非线程安全。多线程环境下需外部加锁。
6. **编码支持**：基于 `std::string`，默认支持 ASCII 和 UTF-8。对于包含 BOM 的文件需预先处理。
7. **错误容忍**：`load()` 在文件打开失败时仅输出 `"fail"` 到 stdout 并返回，不抛异常。调用方应通过检查文件存在性或解析后数据完整性来间接判断加载是否成功。