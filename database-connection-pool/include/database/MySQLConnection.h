#pragma once

#include <jdbc/cppconn/connection.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>
#include <jdbc/mysql_connection.h>
#include <jdbc/mysql_driver.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "IDatabaseConnection.h"
#include "MySQLResultSet.h"

namespace dcp
{

/// MySQL 数据库连接实现
class MySQLConnection : public IDatabaseConnection
{
public:
    MySQLConnection(const ConnectionConfig& cfg);
    ~MySQLConnection() override;

    MySQLConnection(MySQLConnection&& other) noexcept;
    MySQLConnection& operator=(MySQLConnection&& other) noexcept;

    MySQLConnection(const MySQLConnection& other)            = delete;
    MySQLConnection& operator=(const MySQLConnection& other) = delete;

    // ---- IDatabaseConnection 接口实现 ----
    MySQLConnection* connection() override;
    void close() override;
    bool is_valid() override;

    bool execute(const std::string& sql) override;
    int execute_update(const std::string& sql) override;
    int execute_update() override;
    MySQLResultSet* execute_query(const std::string& sql) override;
    MySQLResultSet* execute_query() override;

    void prepare_statement(const std::string& sql) override;

    void set_int(uint16_t idx, int val) override;
    void set_string(uint16_t idx, const std::string& val) override;
    void set_bigint(uint16_t idx, const std::string& val) override;
    void set_blob(uint16_t idx, std::istream* blob) override;
    void set_boolean(uint16_t idx, bool val) override;
    void set_datetime(uint16_t idx, const std::string& val) override;
    void set_double(uint16_t idx, double val) override;
    void set_uint(uint16_t idx, uint32_t val) override;
    void set_int64(uint16_t idx, int64_t val) override;
    void set_uint64(uint16_t idx, uint64_t val) override;
    void set_null(uint16_t idx, int sql_type) override;
    void set_vector(uint16_t idx, const std::vector<float>& val) override;

    // ---- 事务控制 ----
    void begin() override;
    void commit() override;
    void rollback() override;
    void set_transactionIsolation(sql::enum_transaction_isolation level);

    uint64_t last_insert_id() override;

private:
    sql::Connection*        m_conn;
    sql::PreparedStatement* m_pstmt;
    sql::Statement*         m_stmt;
    bool                    m_pre;
    bool                    m_transaction;
};

} // namespace dcp