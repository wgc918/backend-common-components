#include "../include/database/MySQLConnection.h"

#include <jdbc/cppconn/connection.h>
#include <jdbc/mysql_driver.h>

#include <cassert>
#include <string>

namespace dcp
{

MySQLConnection::MySQLConnection(const ConnectionConfig& cfg)
    : IDatabaseConnection(cfg), m_pstmt(nullptr), m_pre(false), m_transaction(false)
{
    sql::mysql::MySQL_Driver* driver    = sql::mysql::get_mysql_driver_instance();
    std::string               host_name = cfg.ip + ":" + std::to_string(cfg.port);

    m_conn = driver->connect(host_name, cfg.user_name, cfg.password);
    m_stmt = m_conn->createStatement();
    m_conn->setSchema(cfg.db_name);

    std::cout << "connection success" << std::endl;
}

MySQLConnection::~MySQLConnection()
{
    close();
}

MySQLConnection::MySQLConnection(MySQLConnection&& other) noexcept
    : IDatabaseConnection(std::move(other)),
      m_conn(other.m_conn),
      m_stmt(other.m_stmt),
      m_pstmt(other.m_pstmt),
      m_pre(other.m_pre),
      m_transaction(other.m_transaction)
{
    other.m_conn        = nullptr;
    other.m_stmt        = nullptr;
    other.m_pstmt       = nullptr;
    other.m_pre         = false;
    other.m_transaction = false;
}

MySQLConnection& MySQLConnection::operator=(MySQLConnection&& other) noexcept
{
    if (this == &other)
        return *this;

    close();

    // 调用基类移动赋值
    IDatabaseConnection::operator=(std::move(other));

    m_conn        = other.m_conn;
    m_stmt        = other.m_stmt;
    m_pstmt       = other.m_pstmt;
    m_pre         = other.m_pre;
    m_transaction = other.m_transaction;

    other.m_conn        = nullptr;
    other.m_stmt        = nullptr;
    other.m_pstmt       = nullptr;
    other.m_pre         = false;
    other.m_transaction = false;

    return *this;
}

// ---- IDatabaseConnection 接口实现 ----
MySQLConnection* MySQLConnection::connection()
{
    if (is_valid())
        return this;
    return nullptr;
}

void MySQLConnection::close()
{
    if (m_pstmt != nullptr)
    {
        delete m_pstmt;
        m_pstmt = nullptr;
    }
    if (m_stmt != nullptr)
    {
        delete m_stmt;
        m_stmt = nullptr;
    }
    if (m_conn != nullptr)
    {
        m_conn->close();
        delete m_conn;
        m_conn = nullptr;
    }
    m_pre         = false;
    m_transaction = false;
}

bool MySQLConnection::is_valid()
{
    return (m_conn != nullptr && m_stmt != nullptr);
}

bool MySQLConnection::execute(const std::string& sql)
{
    return m_stmt->execute(sql);
}

int MySQLConnection::execute_update(const std::string& sql)
{
    return m_stmt->executeUpdate(sql);
}

int MySQLConnection::execute_update()
{
    assert(m_pre && m_pstmt != nullptr);
    return m_pstmt->executeUpdate();
}

MySQLResultSet* MySQLConnection::execute_query(const std::string& sql)
{
    auto ret = new MySQLResultSet(m_stmt->executeQuery(sql));
    return ret;
}

MySQLResultSet* MySQLConnection::execute_query()
{
    assert(m_pre && m_pstmt != nullptr);
    auto ret = new MySQLResultSet(m_pstmt->executeQuery());
    return ret;
}

void MySQLConnection::prepare_statement(const std::string& sql)
{
    if (m_pstmt)
        delete m_pstmt;

    m_pstmt = m_conn->prepareStatement(sql);
    m_pre   = true;
}

void MySQLConnection::set_int(uint16_t idx, int val)
{
    assert(m_pre && m_pstmt != nullptr);
    m_pstmt->setInt(idx, val);
}

void MySQLConnection::set_string(uint16_t idx, const std::string& val)
{
    assert(m_pre && m_pstmt != nullptr);
    m_pstmt->setString(idx, val);
}

void MySQLConnection::set_bigint(uint16_t idx, const std::string& val)
{
    assert(m_pre && m_pstmt != nullptr);
    m_pstmt->setBigInt(idx, val);
}

void MySQLConnection::set_blob(uint16_t idx, std::istream* blob)
{
    assert(m_pre && m_pstmt != nullptr);
    m_pstmt->setBlob(idx, blob);
}

void MySQLConnection::set_boolean(uint16_t idx, bool val)
{
    assert(m_pre && m_pstmt != nullptr);
    m_pstmt->setBoolean(idx, val);
}

void MySQLConnection::set_datetime(uint16_t idx, const std::string& val)
{
    assert(m_pre && m_pstmt != nullptr);
    m_pstmt->setDateTime(idx, val);
}

void MySQLConnection::set_double(uint16_t idx, double val)
{
    assert(m_pre && m_pstmt != nullptr);
    m_pstmt->setDouble(idx, val);
}

void MySQLConnection::set_uint(uint16_t idx, uint32_t val)
{
    assert(m_pre && m_pstmt != nullptr);
    m_pstmt->setUInt(idx, val);
}

void MySQLConnection::set_int64(uint16_t idx, int64_t val)
{
    assert(m_pre && m_pstmt != nullptr);
    m_pstmt->setInt64(idx, val);
}

void MySQLConnection::set_uint64(uint16_t idx, uint64_t val)
{
    assert(m_pre && m_pstmt != nullptr);
    m_pstmt->setUInt64(idx, val);
}

void MySQLConnection::set_null(uint16_t idx, int sql_type)
{
    assert(m_pre && m_pstmt != nullptr);
    m_pstmt->setNull(idx, sql_type);
}

void MySQLConnection::set_vector(uint16_t idx, const std::vector<float>& val)
{
    assert(m_pre && m_pstmt != nullptr);
    m_pstmt->setVector(idx, val);
}

// ---- 事务控制 ----
void MySQLConnection::begin()
{
    if (m_transaction)
        return;
    m_conn->setAutoCommit(false);
    m_transaction = true;
}

void MySQLConnection::commit()
{
    if (m_transaction)
    {
        m_conn->commit();
        m_conn->setAutoCommit(true);  // 恢复自动提交
        m_transaction = false;
    }
}

void MySQLConnection::rollback()
{
    if (m_transaction)
    {
        m_conn->rollback();
        m_conn->setAutoCommit(true);
        m_transaction = false;
    }
}

void MySQLConnection::set_transactionIsolation(sql::enum_transaction_isolation level)
{
    m_conn->setTransactionIsolation(level);
}

uint64_t MySQLConnection::last_insert_id()
{
    return 0;
}

} // namespace dcp