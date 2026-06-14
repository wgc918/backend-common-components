/**
 * example.cpp — INI 解析器基础用法演示
 *
 * 编译：g++ -std=c++17 -I../include example.cpp ../src/ini.cpp ../src/value.cpp -o example
 * 运行：./example
 */

#include <cassert>
#include <iostream>
#include <string>

#include "include/ini.h"

void demo_load_and_access()
{
    std::cout << "========== 1. 加载 INI 文件并访问键值 ==========\n";

    IniFile& ini = IniFile::instance();
    ini.load("test.ini");

    // 访问节
    auto& db     = ini["database"];
    auto& global = ini["Global"];

    // 字符串访问
    std::string host = db["host"];  // operator std::string()
    std::cout << "database.host     = " << host << "\n";

    // 直接使用 val()
    std::cout << "database.username = " << db["username"].val() << "\n";

    // 整型转换
    int port = db["port"];  // operator int()
    std::cout << "database.port     = " << port << "\n";

    // 浮点数转换
    double timeout = db["timeout"];  // operator double()
    std::cout << "database.timeout  = " << timeout << "\n";

    // 布尔转换（仅 "true" 字符串为 true）
    bool ssl = db["use_ssl"];  // operator bool()
    std::cout << "database.use_ssl  = " << (ssl ? "true" : "false") << "\n";

    // 全局键值
    std::cout << "Global.app_name   = " << global["app_name"].val() << "\n";
    std::cout << "Global.version    = " << global["version"].val() << "\n";
    std::cout << "Global.debug      = " << (bool(global["debug"]) ? "true" : "false") << "\n";

    std::cout << "\n";
}

void demo_type_conversion()
{
    std::cout << "========== 2. Value 类型转换 ==========\n";

    IniFile& ini = IniFile::instance();
    auto&    db  = ini["database"];

    // 直接转换到基本类型
    int         connections = db["max_connections"];
    double      to          = db["timeout"];
    bool        ssl         = db["use_ssl"];
    std::string user        = db["username"];

    std::cout << "max_connections (int)    = " << connections << "\n";
    std::cout << "timeout (double)         = " << to << "\n";
    std::cout << "use_ssl (bool)           = " << (ssl ? "true" : "false") << "\n";
    std::cout << "username (std::string)   = " << user << "\n";

    // 显式静态转换
    auto port = static_cast<int>(db["port"]);
    std::cout << "port (static_cast<int>)  = " << port << "\n";

    std::cout << "\n";
}

void demo_edge_cases()
{
    std::cout << "========== 3. 边界情况 ==========\n";

    IniFile& ini = IniFile::instance();

    // 空值
    std::string pwd = ini["redis"]["password"].val();
    std::cout << "redis.password (空值)    = \"" << pwd << "\"\n";

    // 前后空格已被 trim 去除
    std::cout << "whitespace.leading_spaces_key    = \""
              << ini["whitespace"]["leading_spaces_key"].val() << "\"\n";
    std::cout << "whitespace.both_sides             = \"" << ini["whitespace"]["both_sides"].val()
              << "\"\n";

    // 空节（无键值对）
    auto& empty = ini["empty_section"];
    std::cout << "empty_section 中的键数量 = " << empty.size() << "\n";

    // 特殊字符
    std::cout << "special.url       = " << ini["special"]["url"].val() << "\n";
    std::cout << "special.email     = " << ini["special"]["email"].val() << "\n";
    std::cout << "special.json_like = " << ini["special"]["json_like"].val() << "\n";

    std::cout << "\n";
}

void demo_error_handling()
{
    std::cout << "========== 4. 错误处理 ==========\n";

    IniFile& ini = IniFile::instance();

    // 访问不存在的节会抛出 std::out_of_range
    try
    {
        auto& s = ini["nonexistent_section"];
        (void)s;  // 不会执行到这里
        std::cout << "（不应该执行到这里）\n";
    }
    catch (const std::out_of_range& e)
    {
        std::cout << "捕获异常: " << e.what() << "\n";
    }

    // 访问不存在的键不会抛异常（unordered_map 默认构造）
    auto& db = ini["database"];
    std::cout << "database.nonexistent_key = \"" << db["nonexistent_key"].val() << "\"\n";
    std::cout << "（不存在的键返回空 Value）\n";

    std::cout << "\n";
}

int main()
{
    demo_load_and_access();
    demo_type_conversion();
    demo_edge_cases();
    demo_error_handling();

    std::cout << "所有示例执行完毕。\n";
    return 0;
}