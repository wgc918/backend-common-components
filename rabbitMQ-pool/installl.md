# RabbitMQ 环境搭建指南

> 一份面向开发与生产的 RabbitMQ 部署及客户端安装手册，兼顾便捷性与可靠性。

---

## 目录

1. [服务器部署](#1-服务器部署)  
   - 1.1 Docker 快速部署（开发/测试）  
   - 1.2 裸机/VM 生产部署（推荐生产环境）  
2. [客户端库安装（C语言）](#2-客户端库安装c语言)  
   - 2.1 编译安装 rabbitmq-c  
   - 2.2 验证安装  
3. [管理界面与连接验证](#3-管理界面与连接验证)  
4. [常见问题与建议](#4-常见问题与建议)  

---

## 1. 服务器部署

### 1.1 Docker 快速部署（开发/测试）

适用于**本地开发、功能验证或中小型非核心业务**。一键启动，开箱即用。

```bash
docker run -d \
  --name myRabbitMQ \
  -e RABBITMQ_DEFAULT_USER=zsr \
  -e RABBITMQ_DEFAULT_PASS=123456 \
  -p 15672:15672 \
  -p 5672:5672 \
  rabbitmq:3.8.14-management
```

**参数说明**

| 参数 | 说明 |
|------|------|
| `-d` | 后台运行容器 |
| `--name` | 容器名称，便于管理 |
| `-e RABBITMQ_DEFAULT_USER` | 默认管理员用户名 |
| `-e RABBITMQ_DEFAULT_PASS` | 默认管理员密码 |
| `-p 15672:15672` | 映射管理插件 Web 端口 |
| `-p 5672:5672` | 映射 AMQP 协议通信端口 |
| `rabbitmq:3.8.14-management` | 镜像版本（含管理插件） |

启动后可通过 `http://<宿主机IP>:15672` 访问 Web 管理界面，用上面设置的用户名/密码登录。

---

### 1.2 裸机/VM 生产部署（推荐生产环境）

> ⚠️ **大型/核心生产环境，建议采用裸机或虚拟机部署，避免 Docker 带来的网络、存储、监控及升级复杂性。**

以下以 **Ubuntu 22.04 LTS** 为例，其他发行版类似。

#### 1.2.1 安装 Erlang 依赖

RabbitMQ 基于 Erlang，需先安装匹配版本（[版本兼容性参考](https://www.rabbitmq.com/which-erlang.html)）。

```bash
# 添加 Erlang 官方源
wget -O- https://packages.erlang-solutions.com/ubuntu/erlang_solutions.asc | sudo apt-key add -
echo "deb https://packages.erlang-solutions.com/ubuntu focal contrib" | sudo tee /etc/apt/sources.list.d/erlang.list
sudo apt update
sudo apt install -y erlang-base erlang-nox erlang-dev
```

#### 1.2.2 安装 RabbitMQ Server

```bash
# 添加 RabbitMQ 官方源
curl -1sLf "https://keys.openpgp.org/vks/v1/by-fingerprint/0A9AF2115F4687BD29803A206B73A36E602467DF" | sudo gpg --dearmor | sudo tee /usr/share/keyrings/com.rabbitmq.team.gpg > /dev/null
sudo tee /etc/apt/sources.list.d/rabbitmq.list <<EOF
deb [signed-by=/usr/share/keyrings/com.rabbitmq.team.gpg] https://ppa1.novemberain.com/rabbitmq/rabbitmq-erlang/deb/ubuntu jammy main
deb [signed-by=/usr/share/keyrings/com.rabbitmq.team.gpg] https://ppa2.novemberain.com/rabbitmq/rabbitmq-server/deb/ubuntu jammy main
EOF
sudo apt update
sudo apt install -y rabbitmq-server
```

#### 1.2.3 启动并启用管理插件

```bash
sudo systemctl enable rabbitmq-server
sudo systemctl start rabbitmq-server

# 启用管理 Web 插件
sudo rabbitmq-plugins enable rabbitmq_management
```

#### 1.2.4 创建管理员用户（安全建议）

```bash
# 添加用户（替换为强密码）
sudo rabbitmqctl add_user zsr <强密码>
# 设置为管理员
sudo rabbitmqctl set_user_tags zsr administrator
# 授予所有虚拟主机权限
sudo rabbitmqctl set_permissions -p / zsr ".*" ".*" ".*"
# 删除默认 guest 用户（生产强烈建议）
sudo rabbitmqctl delete_user guest
```

---

## 2. 客户端库安装（C语言）

若你需要使用 **C 语言** 与 RabbitMQ 交互，官方推荐 `rabbitmq-c` 库。

### 2.1 编译安装 rabbitmq-c

```bash
# 1. 克隆源码（也可下载压缩包）
git clone https://github.com/alanxz/rabbitmq-c.git
cd rabbitmq-c

# 2. 创建编译目录
mkdir build && cd build

# 3. 配置（指定安装前缀，默认 /usr/local）
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local

# 4. 编译（-j 线程数视 CPU 核心数调整）
make -j4

# 5. 安装（需要 sudo）
sudo make install
```

**依赖提示**：若编译报错缺少 OpenSSL 或 PCM，请先安装：
```bash
sudo apt install libssl-dev libpam0g-dev  # Ubuntu/Debian
# 或
sudo yum install openssl-devel pam-devel  # CentOS/RHEL
```

### 2.2 验证安装

编译完成后，库文件默认安装至 `/usr/local/lib`，头文件在 `/usr/local/include/rabbitmq-c/`。

可编写简单测试程序（`test.c`）验证：

```c
#include <amqp.h>
#include <amqp_tcp_socket.h>
#include <stdio.h>

int main() {
    amqp_connection_state_t conn = amqp_new_connection();
    amqp_socket_t *socket = amqp_tcp_socket_new(conn);
    if (!socket) {
        printf("创建 socket 失败\n");
        return 1;
    }
    int status = amqp_socket_open(socket, "127.0.0.1", 5672);
    if (status) {
        printf("连接失败\n");
        return 1;
    }
    printf("连接成功！\n");
    amqp_destroy_connection(conn);
    return 0;
}
```

编译运行：
```bash
gcc -o test test.c -lrabbitmq -lpthread
./test
```

输出 `连接成功！` 即表示客户端库工作正常。

---

## 3. 管理界面与连接验证

- **Web 管理界面**：`http://<服务器IP>:15672`，使用管理员账户登录后可查看队列、交换器、连接状态等。
- **命令行工具**：
  ```bash
  # 查看集群状态
  sudo rabbitmqctl cluster_status
  # 查看队列列表
  sudo rabbitmqctl list_queues
  # 查看连接
  sudo rabbitmqctl list_connections
  ```
- **连接测试（Python 示例）**：
  ```python
  import pika
  connection = pika.BlockingConnection(pika.ConnectionParameters(
      host='服务器IP', port=5672, credentials=pika.PlainCredentials('zsr', '123456')))
  channel = connection.channel()
  channel.queue_declare(queue='test')
  channel.basic_publish(exchange='', routing_key='test', body='Hello')
  print("发送成功")
  connection.close()
  ```

---

## 4. 常见问题与建议

| 问题 | 解决方案 |
|------|----------|
| **端口无法访问** | 检查防火墙（`ufw allow 5672/tcp`、`15672/tcp`），云服务安全组放行。 |
| **Docker 容器重启丢失数据** | 挂载持久化卷：`-v /data/rabbitmq:/var/lib/rabbitmq`。 |
| **生产环境内存/磁盘告警** | 调整 `rabbitmq.conf` 中的 `vm_memory_high_watermark` 和 `disk_free_limit`。 |
| **默认 guest 用户安全风险** | 生产务必删除或禁用 guest（仅限 localhost 登录）。 |
| **Erlang 版本不匹配** | 参考 [官方版本矩阵](https://www.rabbitmq.com/which-erlang.html) 安装对应 Erlang。 |
| **编译 rabbitmq-c 报错** | 确保已安装 OpenSSL 开发包，并检查 CMake 版本（>=3.0）。 |

---

## 总结

- **开发/测试** → Docker 一键部署，快速迭代。  
- **生产环境** → 裸机/VM 安装，精细化监控与运维。  
- **C 语言客户端** → 通过 `rabbitmq-c` 编译安装，简单易用。  

按照本指南操作，即可轻松搭建稳定、安全的 RabbitMQ 服务环境。如有更深层需求，请查阅 [官方文档](https://www.rabbitmq.com/documentation.html)。

---
*文档版本：1.0 | 最后更新：2026-07-30*