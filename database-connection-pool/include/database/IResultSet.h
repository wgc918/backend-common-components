#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

/// 查询结果集抽象接口
/// 提供统一的游标遍历和字段读取能力
class IResultSet
{
public:
    IResultSet()                              = default;
    IResultSet(IResultSet&& other)            = default;
    IResultSet& operator=(IResultSet&& other) = default;
    virtual ~IResultSet()                     = default;

    IResultSet(const IResultSet& other)            = delete;
    IResultSet& operator=(const IResultSet& other) = delete;

    /// 游标移动到下一行
    /// @return true 存在下一行，false 已到末尾
    virtual bool next() = 0;

    /// 关闭结果集，释放资源
    virtual void close() = 0;

    /// 获取结果集列数
    virtual std::size_t get_column_count() = 0;

    // ---- 按列名读取 ----
    virtual int get_int(const std::string& column_name)            = 0;
    virtual uint32_t get_uint(const std::string& column_name)      = 0;
    virtual int64_t get_int64(const std::string& column_name)      = 0;
    virtual uint64_t get_uint64(const std::string& column_name)    = 0;
    virtual double get_double(const std::string& column_name)      = 0;
    virtual float get_float(const std::string& column_name)        = 0;
    virtual std::string get_string(const std::string& column_name) = 0;
    virtual bool get_boolean(const std::string& column_name)       = 0;

    // ---- 按列索引读取（从 0 开始） ----
    virtual int get_int(uint16_t column_index)            = 0;
    virtual uint32_t get_uint(uint16_t column_index)      = 0;
    virtual int64_t get_int64(uint16_t column_index)      = 0;
    virtual uint64_t get_uint64(uint16_t column_index)    = 0;
    virtual double get_double(uint16_t column_index)      = 0;
    virtual float get_float(uint16_t column_index)        = 0;
    virtual std::string get_string(uint16_t column_index) = 0;
    virtual bool get_boolean(uint16_t column_index)       = 0;

    /// 判断当前行指定列是否为 NULL
    virtual bool is_null(const std::string& column_name) = 0;
    virtual bool is_null(uint16_t column_index)          = 0;
};