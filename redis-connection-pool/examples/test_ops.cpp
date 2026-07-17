#include <sw/redis++/redis++.h>

#include <iostream>
using namespace std;

using namespace sw;

int main()
{
    redis::Redis redis("tcp://127.0.0.1:6379");
    redis.flushdb();
    redis.set("wgc", "hello");
    auto ret = redis.get("wgc");
    cout << *ret << endl;

    return 0;
}