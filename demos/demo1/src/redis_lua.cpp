//-----------------------------------------------------------------------------
// 文件: redis_lua.cpp
// 描述: Redis Lua 脚本原子库存扣减实现
//-----------------------------------------------------------------------------

#include "redis_lua.h"

#include "error_codes.h"
#include "log.h"

namespace demo1
{

const char* RedisLuaManager::deduct_script()
{
    // Lua 脚本：原子性检查并扣减 Redis 库存
    // 返回值: { result_code, new_stock }
    //   result_code: -1=Key不存在, 0=库存不足, 1=扣减成功
    return R"LUA(
local key = KEYS[1]
local deduct_qty = tonumber(ARGV[1])
local current = redis.call('GET', key)

if not current then
    return {-1, 0}
end

current = tonumber(current)
if current < deduct_qty then
    return {0, current}
end

local new_stock = redis.call('DECRBY', key, deduct_qty)
return {1, new_stock}
)LUA";
}

DeductResult RedisLuaManager::parse_result(const std::vector<sw::redis::OptionalString>& reply)
{
    DeductResult result{};
    result.result = LuaResult::KeyNotFound;

    if (reply.size() < 2)
    {
        LOGW("Lua script returned unexpected result size: %zu", reply.size());
        return result;
    }

    if (!reply[0] || !reply[1])
    {
        LOGW("Lua script returned nil values");
        return result;
    }

    int code               = std::stoi(*reply[0]);
    result.remaining_stock = std::stoi(*reply[1]);

    switch (code)
    {
        case -1:
            result.result = LuaResult::KeyNotFound;
            break;
        case 0:
            result.result = LuaResult::StockInsufficient;
            break;
        case 1:
            result.result = LuaResult::DeductSuccess;
            break;
        default:
            LOGW("Lua script returned unknown code: %d", code);
            break;
    }

    return result;
}

DeductResult RedisLuaManager::execute_deduct(sw::redis::Redis& redis, const std::string& key,
                                             int qty)
{
    try
    {
        auto reply = redis.eval<std::vector<sw::redis::OptionalString>>(deduct_script(), {key},
                                                                        {std::to_string(qty)});
        return parse_result(reply);
    }
    catch (const sw::redis::Error& e)
    {
        LOGE("Redis Lua eval failed: %s", e.what());
        return {LuaResult::KeyNotFound, 0};
    }
}

}  // namespace demo1