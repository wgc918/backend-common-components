//-----------------------------------------------------------------------------
// 文件: error_codes.h
// 描述: Demo 业务自定义错误码，使用 X-Macro 模式定义
//       通过 EF_GEN_ERROR_CODES / EF_GEN_ERROR_META 生成常量和查表函数
//-----------------------------------------------------------------------------

#pragma once

#include "ef.h"

// ============================================================
// X-Macro 格式: (code, name, message, level)
// ============================================================
#define DEMO_ERRORS(X)                                  \
    X(1001, StockInsufficient, "库存不足", ::ef::ErrorLevel::Avoid) \
    X(1002, RedisKeyNotFound, "Redis Key 不存在", ::ef::ErrorLevel::CBC) \
    X(1003, MySQLUpdateFailed, "MySQL 更新失败", ::ef::ErrorLevel::Fatal) \
    X(1004, RedisUnavailable, "Redis 不可用", ::ef::ErrorLevel::Warning) \
    X(1005, MySQLUnavailable, "MySQL 不可用", ::ef::ErrorLevel::Fatal) \
    X(1006, InitFailed, "初始化失败", ::ef::ErrorLevel::Fatal) \
    X(1007, ConfigInvalid, "配置无效", ::ef::ErrorLevel::Fatal) \
    X(1008, LuaScriptFailed, "Lua 脚本执行失败", ::ef::ErrorLevel::Fatal) \
    X(1009, CompensationFailed, "Redis 补偿回滚失败", ::ef::ErrorLevel::Fatal) \
    X(1010, ConcurrencyTestFailed, "并发测试异常", ::ef::ErrorLevel::Warning)

// 生成错误码常量
EF_GEN_ERROR_CODES(DEMO_ERRORS)

// 生成元数据表与查表函数
EF_GEN_ERROR_META(DEMO_ERRORS)