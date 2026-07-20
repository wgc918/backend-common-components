#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "IResultSet.h"

namespace dcp
{

struct ConnectionConfig
{
    std::string db_name;
    std::string user_name;
    std::string password;
    std::string ip;
    uint16_t    port;
};

/// 数据库连接抽象接口
/// 定义所有数据库连接必须实现的核心操作：连接、执行、事务
class IDatabaseConnection
{
public:
    IDatabaseConnection(const ConnectionConfig& cfg) : m_cfg(cfg)
    {
    }

    virtual ~IDatabaseConnection() = default;

    IDatabaseConnection(IDatabaseConnection&& other)            = default;
    IDatabaseConnection& operator=(IDatabaseConnection&& other) = default;

    IDatabaseConnection(const IDatabaseConnection& other)            = delete;
    IDatabaseConnection& operator=(const IDatabaseConnection& other) = delete;

    /// 建立数据库连接
    /// @return 返回自身指针，支持链式调用
    virtual IDatabaseConnection* connection() = 0;

    /// 关闭连接，释放资源
    virtual void close() = 0;

    /// 检查连接是否有效
    virtual bool is_valid() = 0;

    /// 执行 DDL / DML 语句（无返回结果集）
    virtual bool execute(const std::string& sql) = 0;

    /// 执行 UPDATE / INSERT / DELETE，返回受影响行数
    virtual int execute_update(const std::string& sql) = 0;

    /// 预处理语句后
    virtual int execute_update() = 0;

    /// 执行 SELECT 查询，返回结果集（调用方负责释放）
    virtual IResultSet* execute_query(const std::string& sql) = 0;

    /// 预处理语句后
    virtual IResultSet* execute_query() = 0;

    /// 预处理语句支持
    virtual void prepare_statement(const std::string& sql) = 0;

    /// 事务控制
    virtual void begin()    = 0;
    virtual void commit()   = 0;
    virtual void rollback() = 0;

    /// 获取最后插入的 ID
    virtual uint64_t last_insert_id() = 0;

    /// 参数绑定接口
    virtual void set_int(uint16_t idx, int val)                          = 0;
    virtual void set_string(uint16_t idx, const std::string& val)        = 0;
    virtual void set_bigint(uint16_t idx, const std::string& val)        = 0;
    virtual void set_blob(uint16_t idx, std::istream* blob)              = 0;
    virtual void set_boolean(uint16_t idx, bool val)                     = 0;
    virtual void set_datetime(uint16_t idx, const std::string& val)      = 0;
    virtual void set_double(uint16_t idx, double val)                    = 0;
    virtual void set_uint(uint16_t idx, uint32_t val)                    = 0;
    virtual void set_int64(uint16_t idx, int64_t val)                    = 0;
    virtual void set_uint64(uint16_t idx, uint64_t val)                  = 0;
    virtual void set_null(uint16_t idx, int sql_type)                    = 0;
    virtual void set_vector(uint16_t idx, const std::vector<float>& val) = 0;

protected:
    ConnectionConfig m_cfg;
};

} // namespace dcp
