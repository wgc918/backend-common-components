// ============================================================================
// module/main.cpp — 模块自定义错误使用示例
//
// 演示场景：
//   1. 定义数据库模块错误（db_errors.h）
//   2. 使用模块错误码构造 Error
//   3. 注册外部查表，使框架 LogError 正确显示模块错误
//   4. 跨模块错误传播
//   5. 模块错误 + 框架内置错误混合使用
// ============================================================================

#include "../../include/ef/ef.h"
#include "db_errors.h"
#include <cstdio>
#include <cstdlib>

using namespace ef::errors;

// ---------------------------------------------------------------------------
// 模拟数据库操作
// ---------------------------------------------------------------------------

struct DbConnection {
    bool initialized = false;
};

// 模拟数据库初始化
ef::Result<void> DbInit(DbConnection& conn) {
    // 使用模块自定义错误码
    const char* db_host = std::getenv("DB_HOST");
    if (!db_host || db_host[0] == '\0') {
        return EF_ERROR(db::errors::kNotInitialized, "DB_HOST env not set");
    }

    conn.initialized = true;
    return {};
}

// 模拟查询操作
ef::Result<int> DbQuery(DbConnection& conn, const std::string& sql) {
    // 使用框架内置错误码
    EF_EXPECT_OR(conn.initialized, kInternal, "db not initialized before query");

    if (sql.find("invalid") != std::string::npos) {
        // 使用模块自定义错误码
        return EF_ERROR(db::errors::kQueryTimeout, "query timed out: '%s'", sql.c_str());
    }

    if (sql.find("dup") != std::string::npos) {
        return EF_ERROR(db::errors::kDupKey, "duplicate key for query: '%s'", sql.c_str());
    }

    return 42;  // 模拟查询结果
}

// 模拟事务操作（错误链演示）
ef::Result<void> DbTransaction(DbConnection& conn) {
    EF_EXPECT_OR(conn.initialized, kInternal, "db not initialized");

    // 底层错误：连接断开
    auto conn_err = EF_ERROR(db::errors::kConnectFail, "lost connection during transaction");

    // 上层错误：事务失败
    auto tx_err = EF_ERROR(db::errors::kTransactionFail, "could not commit, rolled back")
                      .caused_by(std::move(conn_err));

    return tx_err;
}

// ---------------------------------------------------------------------------
// 跨模块组合使用
// ---------------------------------------------------------------------------

ef::Result<std::string> ReadAndQuery(DbConnection& conn, const std::string& config_path) {
    // 框架内置错误：文件不存在
    EF_RETURN_IF(config_path.empty(), kInvalidArg, "config_path must not be empty");

    // 模拟：使用框架错误和模块错误混合
    if (config_path == "missing.cfg") {
        return EF_ERROR(kNotFound, "config file '%s' not found", config_path.c_str());
    }

    // 使用 EF_TRY 调用模块内函数
    EF_CHECK(DbInit(conn));
    auto result = EF_TRY(DbQuery(conn, "SELECT * FROM users"));

    return std::string("Query result: ") + std::to_string(result);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    // 注册模块的外部查表函数（让框架识别 db 模块的错误）
    ef::SetExternalLookup(db::LookupError);

    DbConnection conn;

    // --- 测试 1: 模块错误码常量访问 ---
    std::printf("=== Test 1: Module error codes ===\n");
    std::printf("  kConnectFail = %d\n", db::errors::kConnectFail);
    std::printf("  kQueryTimeout = %d\n", db::errors::kQueryTimeout);
    std::printf("  kDupKey = %d\n", db::errors::kDupKey);
    std::printf("  kDeadlock = %d\n\n", db::errors::kDeadlock);

    // --- 测试 2: 模块错误构造和日志 ---
    std::printf("=== Test 2: Module error LogError ===\n");
    auto mod_err = EF_ERROR(db::errors::kConnectFail, "cannot reach 10.0.0.1:5432");
    ef::LogError(mod_err, "  ");
    // 上面应输出：  [Error] ConnectFail (10001) @ ...
    // 而不是 Unknown，因为注册了查表
    std::printf("\n");

    // --- 测试 3: 模块错误链 ---
    std::printf("=== Test 3: Module error chain ===\n");
    auto low  = EF_ERROR(db::errors::kDeadlock, "row lock on table 'users'");
    auto high = EF_ERROR(db::errors::kTransactionFail, "commit attempt #3 failed")
                    .caused_by(std::move(low));
    ef::LogError(high, "  ");
    std::printf("\n");

    // --- 测试 4: EF_TRY 自动传播模块错误 ---
    std::printf("=== Test 4: EF_TRY with module errors ===\n");
    auto r4 = ReadAndQuery(conn, "");
    if (!r4) {
        ef::LogError(r4.unwrap_err(), "  ");  // 框架内置错误
        std::printf("  Test 4 passed: framework error caught\n\n");
    }

    // --- 测试 5: 跨模块混合使用 ---
    std::printf("=== Test 5: Mixed framework + module errors ===\n");
    auto r5 = ReadAndQuery(conn, "missing.cfg");
    if (!r5) {
        ef::LogError(r5.unwrap_err(), "  ");  // 框架内置 kNotFound
        std::printf("  Test 5 passed: kNotFound logged\n\n");
    }

    // --- 测试 6: 模块错误直接使用 ---
    std::printf("=== Test 6: Direct module error usage ===\n");
    auto r6_init = DbInit(conn);
    if (!r6_init) {
        ef::LogError(r6_init.unwrap_err(), "  ");
    }

    conn.initialized = true;  // 手动初始化

    auto r6_query = DbQuery(conn, "SELECT * FROM invalid_table");
    if (!r6_query) {
        ef::LogError(r6_query.unwrap_err(), "  ");
        std::printf("  Test 6 passed: module kQueryTimeout logged\n\n");
    }

    // --- 测试 7: ErrorInfo 表查询 ---
    std::printf("=== Test 7: ErrorInfo table lookup ===\n");
    auto* info = db::LookupError(db::errors::kDupKey);
    if (info) {
        std::printf("  name=%s, code=%d, severity=%d, msg=%s\n",
            info->name, info->code, static_cast<int>(info->severity), info->message);
    }
    // 查一个未注册的错误码
    auto* unknown_info = db::LookupError(99999);
    std::printf("  unknown lookup result: %s\n", unknown_info ? "found" : "nullptr (expected)");
    std::printf("\n");

    // --- 测试 8: value_or 配合模块错误 ---
    std::printf("=== Test 8: value_or with module errors ===\n");
    auto r8 = DbQuery(conn, "SELECT * FROM dup");
    int val = std::move(r8).value_or(-1);
    std::printf("  value_or fallback: %d (expected -1)\n\n", val);

    std::printf("All module tests passed!\n");
    return 0;
}