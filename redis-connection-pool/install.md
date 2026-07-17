# Ubuntu Redis 环境搭配指南（Redis 服务器 + C++ 客户端）

本指南将引导您在 Ubuntu 系统上从源码编译安装 Redis 服务器，并搭建 C++ 开发所需的客户端库（hiredis 与 redis-plus-plus），为后续开发打好基础。

---

## 📋 目录
1. [安装 Redis 服务器](#1-安装-redis-服务器)
2. [安装 C++ 客户端依赖](#2-安装-c-客户端依赖)
   - 2.1 [安装 hiredis](#21-安装-hiredis)
   - 2.2 [编译安装 redis-plus-plus](#22-编译安装-redis-plus-plus)
3. [验证安装](#3-验证安装)
4. [项目编译配置参考](#4-项目编译配置参考)
5. [常见问题与建议](#5-常见问题与建议)

---

## 1. 安装 Redis 服务器

### 1.1 下载源码
从官方发布页下载稳定版源码（示例为 7.4 版本，请根据需要替换）：
```bash
https://download.redis.io/releases/
```

### 1.2 编译
确保系统已安装 `make` 和 `gcc`，然后执行：
```bash
make -j$(nproc)
```
编译完成后，建议先运行测试（可选）：
```bash
make test
```

### 1.3 安装到指定目录
```bash
make install PREFIX=/usr/local/redis
```
此时 Redis 服务端和客户端可执行文件（`redis-server`, `redis-cli` 等）会被安装到 `/usr/local/redis/bin`。

### 1.4 将 Redis 加入 PATH（方便使用）
```bash
echo 'export PATH=/usr/local/redis/bin:$PATH' >> ~/.bashrc
source ~/.bashrc
```

### 1.5 启动 Redis 服务
```bash
# 前台运行（用于测试）
redis-server

# 或后台运行（需先配置 redis.conf，此处略）
redis-server --daemonize yes
```
默认监听 `127.0.0.1:6379`，可用 `redis-cli ping` 验证是否正常（应返回 `PONG`）。

---

## 2. 安装 C++ 客户端依赖

### 2.1 安装 hiredis
`hiredis` 是 Redis 官方的 C 语言客户端库，也是 `redis-plus-plus` 的底层依赖。

```bash
sudo apt update
sudo apt install -y libhiredis-dev
```
安装后头文件位于 `/usr/include/hiredis/`，库文件位于 `/usr/lib/x86_64-linux-gnu/`。

> 若需要最新版本，可手动从 [hiredis GitHub](https://github.com/redis/hiredis) 编译安装，但包管理器版本已足够。

### 2.2 编译安装 redis-plus-plus
`redis-plus-plus` 是基于 `hiredis` 的 C++ 封装库，支持 C++17 及以上。

#### 2.2.1 安装依赖工具
```bash
sudo apt install -y git cmake g++ make
```

#### 2.2.2 克隆源码
```bash
git clone https://github.com/sewenew/redis-plus-plus.git
cd redis-plus-plus
```

#### 2.2.3 编译与安装
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release   # 若需调试可用 Debug
make -j$(nproc)
sudo make install
```
安装后：
- 头文件：`/usr/local/include/sw/redis++/`
- 静态库：`/usr/local/lib/libredis++.a`
- 动态库：`/usr/local/lib/libredis++.so`（若生成）

> **注意**：若 CMake 版本过低（< 3.0），请先升级：
> ```bash
> sudo apt install cmake3   # 或从官网下载新版
> ```

---

## 3. 验证安装

### 3.1 验证 Redis 服务器
```bash
redis-cli ping
# 应输出：PONG
```

### 3.2 验证 C++ 库可用性
创建一个测试文件 `test.cpp`：
```cpp
#include <sw/redis++/redis++.h>
#include <iostream>

int main() {
    try {
        sw::redis::Redis redis("tcp://127.0.0.1:6379");
        auto reply = redis.ping();
        std::cout << "Redis reply: " << reply << std::endl;
    } catch (const sw::redis::Error &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
```
编译：
```bash
g++ -std=c++17 -o test test.cpp \
    -I/usr/local/include \
    /usr/local/lib/libredis++.a \
    /usr/lib/x86_64-linux-gnu/libhiredis.a \
    -pthread
```
运行：
```bash
./test
# 预期输出：Redis reply: PONG
```

---

## 4. 项目编译配置参考

推荐使用 **Makefile** 或 **CMake** 管理项目。以下是一个简单的 Makefile 模板，适用于多源文件项目：

```Cmake
cmake_minimum_required(VERSION 3.16)

# 设置项目名称
project(redis_conn_pool_examples LANGUAGES CXX)

# 指定 C++ 标准版本（C++17）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
#set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 设置编译选项
if(MSVC)
    # MSVC 编译器选项
    add_compile_options(
        /W4                     # 启用警告级别4
        #/WX                     # 将警告视为错误
        /EHsc                   # 启用标准异常处理
        /utf-8                  # 使用UTF-8编码
    )
else()
    # GCC/Clang 编译器选项
    add_compile_options(
        -Wall                   # 启用所有警告
        -Wextra                 # 启用额外警告
        #-Werror                 # 将警告视为错误
        -pedantic               # 启用严格标准检查
    )
endif()

# 包含头文件目录
include_directories(
    ${PROJECT_SOURCE_DIR}         
    ${PROJECT_SOURCE_DIR}/.. 
)


# 添加测试目标
add_executable(test test.cpp)


target_link_libraries(test redis++)

```

**关键参数说明**：
- `-std=c++17`：`redis-plus-plus` 要求 C++17 标准。
- `-g -O0`：调试模式，发布时建议改为 `-O2`。
- 库路径需根据实际安装位置调整（Ubuntu 默认路径如上）。

---

## 5. 常见问题与建议

| 问题现象 | 可能原因 | 解决方案 |
|---------|---------|---------|
| `fatal error: sw/redis++/redis++.h: No such file or directory` | 未找到头文件 | 确认 `sudo make install` 成功，并检查 `/usr/local/include/sw/redis++/` 是否存在；编译时添加 `-I/usr/local/include` |
| `undefined reference to sw::redis::Redis::Redis(...)` | 链接时未包含 `libredis++.a` | 确保链接命令中包含 `/usr/local/lib/libredis++.a` 和 `libhiredis.a` |
| `Could not connect to Redis server at 127.0.0.1:6379` | Redis 未启动或绑定了其他地址 | 启动 Redis：`redis-server`；或修改连接字符串（如 `tcp://192.168.x.x:6379`） |
| CMake 版本过低 | CentOS 自带 2.x | 安装 `cmake3` 或从官网下载新版 CMake |
| 编译 `redis-plus-plus` 时报错 | 缺少 C++17 支持 | 升级 GCC：`sudo apt install g++-9` 并设置 `CXX=g++-9` |

---

## 📚 参考资源
- [Redis 官方下载](https://download.redis.io/releases/)
- [redis-plus-plus GitHub](https://github.com/sewenew/redis-plus-plus)
- [hiredis GitHub](https://github.com/redis/hiredis)
- [Redis 参考博客](https://zhuanlan.zhihu.com/p/1911427041309460364)
- [redis-plus-plus 参考博客](https://www.cnblogs.com/blfbuaa/p/19197635)
---

✅ **完成上述步骤后，您就拥有了一套完整的 Ubuntu Redis 开发环境，可以开始编写 C++ 应用程序与 Redis 交互了。** 如需更深入的使用示例（如各类数据结构操作、集群访问），可参考 [redis-plus-plus 官方示例](https://github.com/sewenew/redis-plus-plus#examples) 或相关博客。