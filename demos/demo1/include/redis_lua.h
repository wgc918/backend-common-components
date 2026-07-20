//-----------------------------------------------------------------------------
// 文件: redis_lua.h
// 描述: Redis Lua 脚本封装 — 原子性库存扣减
//       提供 Lua 脚本的加载、执行及结果封装
//-----------------------------------------------------------------------------

#pragma once

#include <string>
#include <utility>

#include <sw/redis++/redis++.h>

namespace demo1
{

/// @brief Lua 脚本执行结果
enum class LuaResult
{
    StockInsufficient = 0,  // 库存不足
    DeductSuccess     = 1,  // 扣减成功
    KeyNotFound       = -1  // Redis Key 不存在
};

/// @brief Lua 脚本扣减结果结构
struct DeductResult
{
    LuaResult result;
    int       remaining_stock;  // 扣减后剩余库存（仅成功时有效）
};

/// @brief Redis Lua 脚本管理器
/// 封装库存扣减 Lua 脚本，提供原子性保证
class RedisLuaManager
{
public:
    RedisLuaManager()          = default;
    virtual ~RedisLuaManager() = default;

    /// @brief 获取 Lua 脚本内容
    static const char* deduct_script();

    /// @brief 解析 Lua 脚本返回值
    /// @param reply  Redis Eval 返回的 vector
    /// @return 解析后的扣减结果
    static DeductResult parse_result(const std::vector<sw::redis::OptionalString>& reply);

    /// @brief 执行原子库存扣减
    /// @param redis Redis 连接
    /// @param key   库存 Key
    /// @param qty   扣减数量
    /// @return 扣减结果
    static DeductResult execute_deduct(sw::redis::Redis& redis, const std::string& key, int qty);
};

}  // namespace demo1