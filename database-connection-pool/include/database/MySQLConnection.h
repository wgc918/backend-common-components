#pragma once

#include <jdbc/cppconn/connection.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/mysql_driver.h>

#include <cstdint>
#include <string>

#include "IDatabaseConnection.h"
#include "MySQLResultSet.h"

/// MySQL 数据库连接实现
class MySQLConnection : public IDatabaseConnection
{
public:
    MySQLConnection(const ConnectionConfig& cfg);
    ~MySQLConnection() override;

    // ---- IDatabaseConnection 接口实现 ----
    MySQLConnection* connection(const ConnectionConfig& cfg) override;
    void close() override;
    bool is_valid() override;

    bool execute(const std::string& sql) override;
    int execute_update(const std::string& sql) override;
    MySQLResultSet* execute_query(const std::string& sql) override;

    void prepare_statement(const std::string& sql) override;

    void set_int(uint16_t idx, int val);
    void set_string(uint16_t idx, const std::string& val);

    // ---- 事务控制 ----
    void begin() override;
    void commit() override;
    void rollback() override;

    uint64_t last_insert_id() override;

private:
    sql::Connection*        m_conn;
    sql::PreparedStatement* m_pstmt;
    sql::Statement*         m_stmt;
};