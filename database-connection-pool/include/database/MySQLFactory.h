#pragma once
#include "IConnectionFactory.h"
#include "MySQLConnection.h"

class MySQLFactory : public IConnectionFactory
{
public:
    ~MySQLFactory() override = default;

    /// 根据配置创建数据库连接
    /// @param cfg 连接配置参数
    /// @return MySQL数据库连接对象（调用方负责释放）
    MySQLConnection* create_connection(const ConnectionConfig& cfg) override;

    /// 返回当前工厂对应的数据库类型
    DatabaseType get_database_type() const override;
};
