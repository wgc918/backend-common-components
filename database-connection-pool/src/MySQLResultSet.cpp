#include "../include/database/MySQLResultSet.h"

#include <cstddef>

MySQLResultSet::MySQLResultSet(sql::ResultSet* res) : m_res(res)
{
}

MySQLResultSet::~MySQLResultSet()
{
    if (m_res != nullptr)
    {
        m_res->close();
        delete m_res;
    }
}

MySQLResultSet::MySQLResultSet(MySQLResultSet&& other) : m_res(other.m_res)
{
    other.m_res = nullptr;
}

MySQLResultSet& MySQLResultSet::operator=(MySQLResultSet&& other)
{
    if (this == &other)
        return *this;

    m_res       = other.m_res;
    other.m_res = nullptr;

    return *this;
}

// ---- IResultSet 接口实现 ----
bool MySQLResultSet::next()
{
    return m_res->next();
}

void MySQLResultSet::close()
{
    m_res->close();
}

std::size_t MySQLResultSet::get_column_count()
{
    return m_res->getMetaData()->getColumnCount();
}

int MySQLResultSet::get_int(const std::string& column_name)
{
    return m_res->getInt(column_name);
}

uint32_t MySQLResultSet::get_uint(const std::string& column_name)
{
    return m_res->getUInt(column_name);
}

int64_t MySQLResultSet::get_int64(const std::string& column_name)
{
    return m_res->getInt64(column_name);
}

uint64_t MySQLResultSet::get_uint64(const std::string& column_name)
{
    return m_res->getUInt64(column_name);
}

double MySQLResultSet::get_double(const std::string& column_name)
{
    return m_res->getDouble(column_name);
}

float MySQLResultSet::get_float(const std::string& column_name)
{
    return static_cast<float>(m_res->getDouble(column_name));
}

std::string MySQLResultSet::get_string(const std::string& column_name)
{
    return m_res->getString(column_name);
}

bool MySQLResultSet::get_boolean(const std::string& column_name)
{
    return m_res->getBoolean(column_name);
}

int MySQLResultSet::get_int(uint16_t column_index)
{
    return m_res->getInt(column_index);
}

uint32_t MySQLResultSet::get_uint(uint16_t column_index)
{
    return m_res->getUInt(column_index);
}

int64_t MySQLResultSet::get_int64(uint16_t column_index)
{
    return m_res->getInt64(column_index);
}

uint64_t MySQLResultSet::get_uint64(uint16_t column_index)
{
    return m_res->getUInt64(column_index);
}

double MySQLResultSet::get_double(uint16_t column_index)
{
    return m_res->getDouble(column_index);
}

float MySQLResultSet::get_float(uint16_t column_index)
{
    return static_cast<float>(m_res->getDouble(column_index));
}

std::string MySQLResultSet::get_string(uint16_t column_index)
{
    return m_res->getString(column_index);
}

bool MySQLResultSet::get_boolean(uint16_t column_index)
{
    return m_res->getBoolean(column_index);
}

bool MySQLResultSet::is_null(const std::string& column_name)
{
    return m_res->isNull(column_name);
}

bool MySQLResultSet::is_null(uint16_t column_index)
{
    return m_res->isNull(column_index);
}