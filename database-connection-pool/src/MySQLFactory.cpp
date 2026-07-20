#include "../include/database/MySQLFactory.h"

namespace dcp
{

MySQLConnection* MySQLFactory::create_connection(const ConnectionConfig& cfg)
{
    return new MySQLConnection(cfg);
}

/// 返回当前工厂对应的数据库类型
DatabaseType MySQLFactory::get_database_type() const
{
    return DatabaseType::MySQL;
}

} // namespace dcp