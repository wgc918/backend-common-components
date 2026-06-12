/**
 * advanced_usage.cpp — INI 解析器进阶用法演示
 *
 * 编译：g++ -std=c++17 -I../include advanced_usage.cpp ../src/ini.cpp ../src/value.cpp -o
 * advanced_usage 运行：./advanced_usage
 */

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "../include/ini.h"

// ============================================================
// 5. 遍历所有节与键值
// ============================================================
void demo_iteration()
{
    std::cout << "========== 5. 遍历所有节与键值 ==========\n";

    IniFile& ini = IniFile::instance();
    ini.show();  // 内置打印函数

    std::cout << "\n--- 手动遍历 ---\n";
    // 手动遍历需要将 show() 逻辑展开，此处演示通过已知节名遍历
    for (const auto& section_name : {"Global", "database", "redis", "logging", "network", "special",
                                     "whitespace", "bounds", "cache", "empty_section"})
    {
        try
        {
            auto& sec = ini[section_name];
            std::cout << "[" << section_name << "] (" << sec.size() << " keys)\n";
            for (const auto& kv : sec)
            {
                std::cout << "  " << kv.first << " = " << kv.second.val() << "\n";
            }
        }
        catch (const std::out_of_range&)
        {
            // 跳过不存在的节
        }
    }

    std::cout << "\n";
}

// ============================================================
// 6. 多个 INI 文件加载（后者覆盖前者）
// ============================================================
void demo_multi_file_load()
{
    std::cout << "========== 6. 多次加载（单例模式） ==========\n";

    IniFile& ini = IniFile::instance();

    // 单例模式下，load() 会清空之前的数据再加载
    ini.load("test.ini");
    std::cout << "加载 test.ini 后 database.port = " << static_cast<int>(ini["database"]["port"])
              << "\n";

    // 注意：重复调用 load() 会清空之前的数据
    // 如需合并多个文件，需自行实现

    std::cout << "\n";
}

// ============================================================
// 7. 数值边界处理
// ============================================================
void demo_numeric_bounds()
{
    std::cout << "========== 7. 数值边界 ==========\n";

    IniFile& ini    = IniFile::instance();
    auto&    bounds = ini["bounds"];

    int max_int = bounds["max_int"];
    int min_int = bounds["min_int"];
    int zero    = bounds["zero"];

    double max_double   = bounds["max_double"];
    double min_double   = bounds["min_double"];
    double negative_num = bounds["negative_num"];

    std::cout << "max_int      = " << max_int << "\n";
    std::cout << "min_int      = " << min_int << "\n";
    std::cout << "zero         = " << zero << "\n";
    std::cout << "max_double   = " << max_double << "\n";
    std::cout << "min_double   = " << min_double << "\n";
    std::cout << "negative_num = " << negative_num << "\n";

    std::cout << "\n";
}

// ============================================================
// 8. 配置热加载模式
// ============================================================
void demo_hot_reload()
{
    std::cout << "========== 8. 模拟配置热加载 ==========\n";

    IniFile& ini = IniFile::instance();

    // 第一次加载
    ini.load("test.ini");
    std::string old_level = ini["logging"]["level"].val();
    std::cout << "首次加载 logging.level = " << old_level << "\n";

    // 模拟重新加载（实际场景中可配合文件监控）
    ini.load("test.ini");
    std::string new_level = ini["logging"]["level"].val();
    std::cout << "重新加载 logging.level = " << new_level << "\n";
    std::cout << "（值不变，因为文件未修改；但 load() 会清空并重新解析）\n";

    std::cout << "\n";
}

// ============================================================
// 9. 辅助函数：安全获取配置值（带默认值）
// ============================================================
std::string get_config_str(const std::string& section, const std::string& key,
                           const std::string& default_val = "")
{
    try
    {
        auto& sec = IniFile::instance()[section];
        auto  it  = sec.find(key);
        if (it != sec.end() && !it->second.val().empty())
            return it->second.val();
    }
    catch (const std::out_of_range&)
    {
    }
    return default_val;
}

int get_config_int(const std::string& section, const std::string& key, int default_val = 0)
{
    try
    {
        auto& sec = IniFile::instance()[section];
        auto  it  = sec.find(key);
        if (it != sec.end())
            return static_cast<int>(it->second);
    }
    catch (const std::out_of_range&)
    {
    }
    return default_val;
}

void demo_safe_access()
{
    std::cout << "========== 9. 安全访问（带默认值） ==========\n";

    IniFile& ini = IniFile::instance();
    ini.load("test.ini");

    std::string host    = get_config_str("database", "host", "localhost");
    int         port    = get_config_int("database", "port", 3306);
    std::string missing = get_config_str("database", "nonexistent", "default_value");

    std::cout << "database.host          = " << host << "\n";
    std::cout << "database.port          = " << port << "\n";
    std::cout << "database.nonexistent   = " << missing << " (使用默认值)\n";

    std::cout << "\n";
}

int main()
{
    demo_iteration();
    demo_multi_file_load();
    demo_numeric_bounds();
    demo_hot_reload();
    demo_safe_access();

    std::cout << "所有进阶示例执行完毕。\n";
    return 0;
}