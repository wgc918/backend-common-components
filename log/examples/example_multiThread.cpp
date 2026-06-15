#include <chrono>
#include <thread>

#include "include/log.h"

void func1(int id)
{
    LOGD("d thread_id %d", id);
    LOGI("i thread_id %d", id);
    LOGE("e thread_id %d", id);
    LOGW("w thread_id %d", id);
    LOGF("f thread_id %d", id);
}

int main()
{
    Logger& log = Logger::instance();
    log.init(LogConfig::console_only());

    for (int i = 0; i < 10; i++)
    {
        std::thread th(func1, i);
        th.detach();
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}