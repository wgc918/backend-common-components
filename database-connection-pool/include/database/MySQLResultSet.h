#pragma once
#include <jdbc/cppconn/resultset.h>

#include <cstddef>
#include <cstdint>
#include <string>

#include "IResultSet.h"

namespace dcp
{

/// MySQL 查询结果集实现
class MySQLResultSet : public IResultSet
{
public:
    MySQLResultSet(sql::ResultSet* res);
    ~MySQLResultSet() override;

    MySQLResultSet(MySQLResultSet&& other);
    MySQLResultSet& operator=(MySQLResultSet&& other);

    MySQLResultSet(const MySQLResultSet& other)            = delete;
    MySQLResultSet& operator=(const MySQLResultSet& other) = delete;

    // ---- IResultSet 接口实现 ----
    bool next() override;
    void close() override;
    std::size_t get_column_count() override;

    int get_int(const std::string& column_name) override;
    uint32_t get_uint(const std::string& column_name) override;
    int64_t get_int64(const std::string& column_name) override;
    uint64_t get_uint64(const std::string& column_name) override;
    double get_double(const std::string& column_name) override;
    float get_float(const std::string& column_name) override;
    std::string get_string(const std::string& column_name) override;
    bool get_boolean(const std::string& column_name) override;

    int get_int(uint16_t column_index) override;
    uint32_t get_uint(uint16_t column_index) override;
    int64_t get_int64(uint16_t column_index) override;
    uint64_t get_uint64(uint16_t column_index) override;
    double get_double(uint16_t column_index) override;
    float get_float(uint16_t column_index) override;
    std::string get_string(uint16_t column_index) override;
    bool get_boolean(uint16_t column_index) override;

    bool is_null(const std::string& column_name) override;
    bool is_null(uint16_t column_index) override;

private:
    sql::ResultSet* m_res;
};

} // namespace dcp