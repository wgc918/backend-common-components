跨域通信通用组件架构设计
1. 设计目标
简单易用：提供统一、简洁的 API，隐藏底层 socket 系统调用。

模式灵活：支持 本机通信（AF_UNIX） 与 跨机网络（AF_INET） 地址族，以及 TCP（SOCK_STREAM） 与 UDP（SOCK_DGRAM） 传输协议的自由组合。

高内聚低耦合：各模块职责单一，通过抽象接口交互，便于扩展和维护。

2. 整体分层架构
text
┌─────────────────────────────────────────────┐
│           应用层（用户代码）                  │
└─────────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────┐
│         统一通信接口（Communicator）          │
│  - connect / bind / listen / accept          │
│  - send / recv / sendTo / recvFrom           │
│  - close / getLocalAddress / getPeerAddress  │
└─────────────────────────────────────────────┘
                     │
        ┌────────────┼────────────┐
        ▼            ▼            ▼
┌────────────┐ ┌────────────┐ ┌────────────┐
│ 地址抽象层  │ │ 协议策略层 │ │ 系统调用   │
│ (Address)  │ │(Tcp/Udp    │ │ 封装层     │
│            │ │ 策略)      │ │(SocketOps) │
└────────────┘ └────────────┘ └────────────┘
        │            │            │
        └────────────┴────────────┘
                     │
                     ▼
            操作系统 Socket API
3. 模块划分与核心类设计
3.1 地址抽象模块（Address）
职责：屏蔽 AF_UNIX 路径和 AF_INET 的 IP+Port 差异。

基类 Address

纯虚方法 getSockAddr()（返回 sockaddr* 及长度）和 toString()。

派生类：

UnixAddress：存储 Unix 域路径（std::string path）。

InetAddress：存储 IPv4/IPv6 地址和端口（std::string ip, uint16_t port）。

工厂方法：Address::create(domain, params) 可简化创建。

3.2 系统调用封装模块（SocketOps）
职责：封装所有底层系统调用（socket, bind, connect, send, recv 等），统一错误处理（返回错误码或抛异常）。

静态方法：例如 static int createSocket(int domain, int type, int protocol)、static int bindSocket(int fd, const Address& addr) 等。

目的：使 Communicator 不直接依赖系统调用，提高可测试性。

3.3 通信器核心类（Communicator）
职责：对外提供完整的通信操作，内部根据协议类型（TCP/UDP）动态委托策略，根据地址族构造合适地址。

成员变量：

int fd_：套接字描述符。

int domain_：地址族（AF_UNIX / AF_INET）。

int protocol_：协议类型（SOCK_STREAM / SOCK_DGRAM）。

std::unique_ptr<ProtocolStrategy> strategy_：协议策略对象。

关键方法：

bool connect(const Address& addr)：TCP 连接，UDP 若调用则设置默认对端。

bool bind(const Address& addr)：绑定本地地址。

bool listen(int backlog)：仅 TCP 有效。

Communicator* accept()：仅 TCP 有效，返回新连接对象。

ssize_t send(const void* data, size_t len)：TCP 直接发送；UDP 若已连接则发送给默认对端。

ssize_t recv(void* buf, size_t len)：类似。

ssize_t sendTo(const Address& addr, const void* data, size_t len)：UDP 发送到指定地址。

ssize_t recvFrom(Address& addr, void* buf, size_t len)：UDP 接收并获取来源地址。

void close()：关闭套接字。

内部策略委托：对于 TCP 特有操作（如 listen/accept），调用 strategy_->listen(this) 等，UDP 策略则返回错误或抛出 ProtocolNotSupported 异常。

3.4 协议策略模块（ProtocolStrategy）
抽象基类 ProtocolStrategy

定义接口：listen(Communicator*)、accept(Communicator*)、send(Communicator*, ...) 等。

具体策略：

TcpStrategy：实现 TCP 相关语义（流式阻塞/非阻塞、连接管理等）。

UdpStrategy：实现 UDP 数据报收发，并支持 sendTo/recvFrom。

通信器与策略的协作通过依赖注入（构造时传入策略）实现，降低耦合。

3.5 工厂模块（CommunicatorFactory）
职责：简化对象创建，根据用户指定模式返回合适的 Communicator 实例。

静态方法：
static Communicator create(配置结构体)：通过域和类型创建，内部选择合适的策略。

优势：用户无需关心策略选择和地址构造细节。

4. 高内聚低耦合设计要点
模块	内聚性说明	耦合性说明
Address	仅负责地址表示与转换	只依赖 sockaddr 结构，不依赖其他模块
SocketOps	仅封装系统调用，无业务逻辑	被 Communicator 调用，不反向依赖
Communicator	聚焦于通信流程控制，协调策略和地址	依赖 Address 和 ProtocolStrategy 的抽象接口
ProtocolStrategy	单独封装协议特定行为	依赖 Communicator 接口（只使用其 fd() 等访问器）
Factory	仅负责对象组装	依赖抽象基类，不依赖具体实现
依赖倒置：高层模块（用户代码）依赖 Communicator 抽象，Communicator 依赖 ProtocolStrategy 抽象，不依赖具体实现。

开闭原则：新增协议（如 SCTP）只需新增策略类，无需修改 Communicator 代码。

单一职责：每个类只有一个变更理由，如 Address 只在地址格式变化时修改。