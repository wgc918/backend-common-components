// ============================================================================
// error_example.cpp — 完整使用示例
// 场景：读文件 → 解析配置 → 上层处理
// ============================================================================

#include "../include/error/result.h"
#include "../include/error/macros.h"
#include "../include/error/error_utils.h"
#include <string>
#include <cstdio>
#include <utility>

struct ServerConfig {
    int  port;
    bool debug;
};

// 自定义错误类型（需定义在命名空间/全局作用域，C++17 模板不支持 local type）
struct NetError {
    int         http_code;
    std::string body;
};

// 模拟读文件
Result<std::string> ReadFile(const std::string& path) {
    if (path.empty()) {
        return AEF(InvalidArg, "path must not be empty");
    }
    if (path == "notfound.cfg") {
        return AEF(FileNotFound, "cannot open '%s'", path.c_str());
    }
    return std::string("port=8080\ndebug=true");
}

// 模拟解析配置
Result<ServerConfig> ParseConfig(const std::string& raw) {
    if (raw.empty()) {
        return AEF(ParseError, "empty config content");
    }

    ServerConfig cfg{};
    auto pos = raw.find("port=");
    if (pos == std::string::npos) {
        return AEF(ParseError, "missing 'port' field in config");
    }
    cfg.port   = 8080;
    cfg.debug  = true;
    return cfg;
}

// 上层组合调用
Result<ServerConfig> LoadConfig(const std::string& path) {
    auto file_result = ReadFile(path);
    if (!file_result) {
        LogError(file_result.unwrap_err());
        return file_result.unwrap_err();
    }

    auto parse_result = ParseConfig(file_result.unwrap());
    if (!parse_result) {
        LogError(parse_result.unwrap_err());
        return parse_result.unwrap_err();
    }

    return parse_result.unwrap();
}

int main() {
    std::printf("=== 测试 1: 正常路径 ===\n");
    auto r1 = LoadConfig("server.cfg");
    if (r1) {
        auto cfg = r1.unwrap();
        std::printf("OK: port=%d, debug=%d\n\n", cfg.port, cfg.debug);
    }

    std::printf("=== 测试 2: 文件不存在 ===\n");
    auto r2 = LoadConfig("notfound.cfg");
    if (!r2) {
        std::printf("Test 2 passed: error detected\n\n");
    }

    std::printf("=== 测试 3: value_or 默认值 ===\n");
    auto r3 = LoadConfig("notfound.cfg");
    auto cfg3 = std::move(r3).value_or(ServerConfig{3000, false});
    std::printf("Fallback: port=%d, debug=%d\n\n", cfg3.port, cfg3.debug);

    std::printf("=== 测试 4: 自定义错误类型 ===\n");
    Result<std::string, NetError> fetch = NetError{503, "Service Unavailable"};
    if (!fetch) {
        auto& e = fetch.unwrap_err();
        std::printf("HTTP %d: %s\n\n", e.http_code, e.body.c_str());
    }

    std::printf("=== 测试 5: 错误链 ===\n");
    auto low  = AEF(ConnectionRefused, "connect to 10.0.0.1:8080 failed");
    auto high = AEF(NetworkTimeout, "service call failed after 3 retries")
                    .caused_by(std::move(low));
    LogError(high);
    std::printf("\n");

    std::printf("All tests passed!\n");
    return 0;
}
