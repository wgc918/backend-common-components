#pragma once
#include <cstdint>
#include <string>

#include "IResultSet.h"

struct ConnectionConfig
{
    std::string db_name;
    std::string user_name;
    std::string passward;
    std::string ip;
    uint16_t    port;
};

/// 数据库连接抽象接口
/// 定义所有数据库连接必须实现的核心操作：连接、执行、事务
class IDatabaseConnection
{
public:
    IDatabaseConnection(const ConnectionConfig& cfg);
    virtual ~IDatabaseConnection() = default;

    /// 建立数据库连接
    /// @return 返回自身指针，支持链式调用
    virtual IDatabaseConnection* connection(const ConnectionConfig& cfg) = 0;

    /// 关闭连接，释放资源
    virtual void close() = 0;

    /// 检查连接是否有效
    virtual bool is_valid() = 0;

    /// 执行 DDL / DML 语句（无返回结果集）
    virtual bool execute(const std::string& sql) = 0;

    /// 执行 UPDATE / INSERT / DELETE，返回受影响行数
    virtual int execute_update(const std::string& sql) = 0;

    /// 执行 SELECT 查询，返回结果集（调用方负责释放）
    virtual IResultSet* execute_query(const std::string& sql) = 0;

    /// 预处理语句支持
    virtual void prepare_statement(const std::string& sql) = 0;

    /// 事务控制
    virtual void begin()    = 0;
    virtual void commit()   = 0;
    virtual void rollback() = 0;

    /// 获取最后插入的 ID
    virtual uint64_t last_insert_id() = 0;

protected:
    ConnectionConfig m_cfg;
};
