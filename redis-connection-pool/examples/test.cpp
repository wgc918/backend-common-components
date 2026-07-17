#include <sw/redis++/redis.h>

#include <iostream>

using namespace std;

int main()
{
    // 创建 Redis 对象的时候，需要在构造函数中，指定 redis 服务器的地址和端口
    sw::redis::Redis redis("tcp://127.0.0.1:6379");  // URL 唯一资源定位符
    // 此时咱们的 redis 客户端和服务端是在同一台主机上，使用本地环回就可以了
    // 使用 ping 方法，让客户端给服务器发一个 PING，然后服务器就会返回一个
    // PONG，就通过 返回值 获取到
    string result = redis.ping();
    cout << result << endl;
    return 0;
}