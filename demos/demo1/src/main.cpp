//-----------------------------------------------------------------------------
// 文件: main.cpp
// 描述: 应用入口 — 解析命令行参数，启动 App
//-----------------------------------------------------------------------------

#include <cstdlib>
#include <iostream>

#include "app.h"

int main(int argc, char* argv[])
{
    const char* config_path = "config.ini";
    if (argc > 1)
    {
        config_path = argv[1];
    }

    demo1::App app;

    if (!app.init(config_path))
    {
        std::cerr << "[FATAL] Application initialization failed. Exiting." << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "[INFO] Demo1 Inventory System starting with Drogon..." << std::endl;

    app.run();  // 阻塞，直到 Drogon 停止

    return EXIT_SUCCESS;
}