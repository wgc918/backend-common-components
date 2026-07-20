//-----------------------------------------------------------------------------
// 文件: bootstrap.cpp
// 描述: 应用启动引导 — 库存基础数据初始化
//-----------------------------------------------------------------------------

#include "bootstrap.h"

#include "log.h"

namespace demo1
{

Bootstrap::Bootstrap(IInventoryService& service) : m_service(service)
{
}

Result<void, ef::Error> Bootstrap::run(int item_count, int init_stock)
{
    LOGI("=== Bootstrap: Initializing inventory ===");
    LOGI("  Items: %d, Initial stock per item: %d", item_count, init_stock);

    auto result = m_service.initialize(item_count, init_stock);
    if (!result)
    {
        LOGE("Bootstrap initialization failed: %s", result.unwrap_err().to_string().c_str());
        return result;
    }

    LOGI("=== Bootstrap: Completed ===");
    return Result<void, ef::Error>();
}

}  // namespace demo1