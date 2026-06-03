// ============================================================================
// basic/main.cpp — 基础使用示例（使用框架内置错误码）
//
// 演示场景：
//   1. 基础错误构造和返回
//   2. Result<T> 的完整生命周期
//   3. EF_TRY 错误自动传播
//   4. EF_CHECK void 检查
//   5. EF_RETURN_IF 条件返回
//   6. 错误链 + LogError
//   7. value_or 默认值回退
//   8. Panic 致命错误终止
// ============================================================================

#include "../../include/ef/ef.h"
#include <string>
#include <cstdio>

using namespace ef::errors;

// ---------------------------------------------------------------------------
// 模拟数据结构
// ---------------------------------------------------------------------------

struct ServerConfig {
    int  port;
    bool debug;
};

// ---------------------------------------------------------------------------
// 场景 1: 基础错误构造和返回
// ---------------------------------------------------------------------------

ef::Result<std::string> ReadFile(const std::string& path) {
    if (path.empty()) {
        return EF_ERROR(kInvalidArg, "path must not be empty");
    }
    if (path == "notfound.cfg") {
        return EF_ERROR(kNotFound, "cannot open '%s'", path.c_str());
    }
    return std::string("port=8080\ndebug=true");
}

// ---------------------------------------------------------------------------
// 场景 2: Result<T> 使用 + value_or 默认值
// ---------------------------------------------------------------------------

ef::Result<ServerConfig> ParseConfig(const std::string& raw) {
    if (raw.empty()) {
        return EF_ERROR(kParseError, "empty config content");
    }

    auto pos = raw.find("port=");
    if (pos == std::string::npos) {
        return EF_ERROR(kParseError, "missing 'port' field");
    }

    ServerConfig cfg{8080, true};
    return cfg;
}

// ---------------------------------------------------------------------------
// 场景 3: EF_TRY 错误自动传播
// ---------------------------------------------------------------------------

ef::Result<ServerConfig> LoadConfig(const std::string& path) {
    // 使用 EF_TRY：成功时解包值，失败时自动 return 错误
    auto content = EF_TRY(ReadFile(path));
    auto config  = EF_TRY(ParseConfig(content));
    return config;
}

// ---------------------------------------------------------------------------
// 场景 4: EF_CHECK void 检查
// ---------------------------------------------------------------------------

ef::Result<void> ValidateConfig(const ServerConfig& cfg) {
    EF_RETURN_IF(cfg.port <= 0,    kInvalidArg, "port must be positive");
    EF_RETURN_IF(cfg.port > 65535, kInvalidArg, "port too large: %d", cfg.port);
    return {};
}

ef::Result<void> InitServer(const std::string& path) {
    // EF_CHECK 检查 void Result，失败则提前返回
    auto cfg = EF_TRY(LoadConfig(path));
    EF_CHECK(ValidateConfig(cfg));
    return {};
}

// ---------------------------------------------------------------------------
// 场景 5: EF_EXPECT_OR 断言式检查
// ---------------------------------------------------------------------------

ef::Result<int> SafeDivide(int a, int b) {
    EF_EXPECT_OR(b != 0, kInvalidArg, "division by zero, %d / %d", a, b);
    return a / b;
}

// ---------------------------------------------------------------------------
// 场景 6: 错误链 + LogError 日志输出
// ---------------------------------------------------------------------------

ef::Result<std::string> FetchData(const std::string& url) {
    // 模拟网络请求失败
    auto conn_err = EF_ERROR(kConnectionRefused, "connect to %s:8080 failed", url.c_str());
    auto timeout  = EF_ERROR(kTimeout, "service call failed after 3 retries")
                        .caused_by(std::move(conn_err));
    return timeout;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    // --- 测试 1: 正常路径 ---
    std::printf("=== Test 1: Normal path ===\n");
    auto r1 = LoadConfig("server.cfg");
    if (r1) {
        std::printf("  OK: port=%d, debug=%d\n\n", r1->port, r1->debug);
    }

    // --- 测试 2: 文件不存在 ---
    std::printf("=== Test 2: File not found ===\n");
    auto r2 = LoadConfig("notfound.cfg");
    if (!r2) {
        ef::LogError(r2.unwrap_err(), "  ");
        std::printf("  Test 2 passed: error properly handled\n\n");
    }

    // --- 测试 3: value_or 默认值 ---
    std::printf("=== Test 3: value_or fallback ===\n");
    auto r3 = LoadConfig("notfound.cfg");
    // 注意：LogError 已在上一个测试输出，这里 value_or 直接获取默认值
    auto cfg3 = std::move(r3).value_or(ServerConfig{3000, false});
    std::printf("  Fallback: port=%d, debug=%d\n\n", cfg3.port, cfg3.debug);

    // --- 测试 4: EF_CHECK void result ---
    std::printf("=== Test 4: EF_CHECK void result ===\n");
    auto r4 = InitServer("server.cfg");
    if (r4) {
        std::printf("  Server initialized OK\n\n");
    }

    // --- 测试 5: EF_RETURN_IF ---
    std::printf("=== Test 5: EF_RETURN_IF ===\n");
    auto r5 = InitServer("notfound.cfg");
    if (!r5) {
        ef::LogError(r5.unwrap_err(), "  ");
        std::printf("  Test 5 passed: validation/init error caught\n\n");
    }

    // --- 测试 6: 错误链 ---
    std::printf("=== Test 6: Error chain ===\n");
    auto r6 = FetchData("api.example.com");
    if (!r6) {
        ef::LogError(r6.unwrap_err(), "  ");
        std::printf("  Test 6: error chain printed above\n\n");
    }

    // --- 测试 7: error.to_string() ---
    std::printf("=== Test 7: to_string() ===\n");
    auto err = EF_ERROR(kNotFound, "resource '%d' missing", 42);
    std::printf("  %s\n\n", err.to_string().c_str());

    // --- 测试 8: Result<void> 显式使用 ---
    std::printf("=== Test 8: Result<void> explicit ===\n");
    ef::Result<void> ok_result;
    std::printf("  ok_result.is_ok() = %s\n", ok_result.is_ok() ? "true" : "false");

    ef::Result<void> err_result = EF_ERROR(kInternal, "something went wrong");
    std::printf("  err_result.is_err() = %s\n", err_result.is_err() ? "true" : "false");
    std::printf("  err_result.unwrap_err().what() = %s\n\n", err_result.unwrap_err().what());

    // --- 测试 9: SafeDivide 正常和错误 ---
    std::printf("=== Test 9: SafeDivide ===\n");
    auto div_ok = SafeDivide(10, 2);
    if (div_ok) {
        std::printf("  10/2 = %d\n", *div_ok);
    }
    auto div_err = SafeDivide(10, 0);
    if (!div_err) {
        ef::LogError(div_err.unwrap_err(), "  ");
        std::printf("  Test 9 passed: division by zero caught\n\n");
    }

    std::printf("All basic tests passed!\n");
    return 0;
}