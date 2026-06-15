#include "include/log.h"

void func1()
{
    LOGD("d hello word!");
    LOGI("i hello word!");
    LOGE("e hello word!");
    LOGW("w hello word!");
    LOGF("f hello word!");
}

void func2()
{
    auto    now    = std::chrono::system_clock::now();
    auto    time_t = std::chrono::system_clock::to_time_t(now);
    std::tm local_time;
    localtime_r(&time_t, &local_time);
    LOGD("d today is %d年-%d月-%d日", local_time.tm_year + 1900, local_time.tm_mon + 1,
         local_time.tm_mday);
    LOGI("i today is %d年-%d月-%d日", local_time.tm_year + 1900, local_time.tm_mon + 1,
         local_time.tm_mday);
    LOGE("e today is %d年-%d月-%d日", local_time.tm_year + 1900, local_time.tm_mon + 1,
         local_time.tm_mday);
    LOGW("w today is %d年-%d月-%d日", local_time.tm_year + 1900, local_time.tm_mon + 1,
         local_time.tm_mday);
    LOGF("f today is %d年-%d月-%d日", local_time.tm_year + 1900, local_time.tm_mon + 1,
         local_time.tm_mday);
}

void func3(int id)
{
    LOGD("d hello word!", id);
    LOGI("i hello word!", id);
    LOGE("e hello word!", id);
    LOGW("w hello word!", id);
    LOGF("f hello word!", id);
}

int main()
{
    Logger& log = Logger::instance();
    // log.init(LogConfig::console_only());
    log.init(LogConfig::file_only("./test.log"));
    func1();
    func2();
    func3(999);

    return 0;
}