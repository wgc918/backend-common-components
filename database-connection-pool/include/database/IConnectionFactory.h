#pragma once
#include <memory>

#include "DatabaseType.h"
#include "IDatabaseConnection.h"

/// 连接工厂抽象接口
/// 每种数据库类型对应一个具体工厂实现，负责创建对应的数据库连接
class IConnectionFactory
{
public:
    virtual ~IConnectionFactory() = default;

    /// 根据配置创建数据库连接
    /// @param cfg 连接配置参数
    /// @return 数据库连接对象（调用方负责释放）
    virtual IDatabaseConnection* create_connection(const ConnectionConfig& cfg) = 0;

    /// 返回当前工厂对应的数据库类型
    virtual DatabaseType get_database_type() const = 0;
};