#include "../include/value.h"

#include <string>

Value::Value(bool val)
{
    m_val = val ? "true" : "false";
}

Value::Value(int val) : m_val(std::to_string(val))
{
}

Value::Value(double val) : m_val(std::to_string(val))
{
}

Value::Value(const std::string& val) : m_val(val)
{
}

Value::Value(std::string&& val) : m_val(std::move(val))
{
}

Value::Value(const char* val) : m_val()
{
    if (val != nullptr)
        m_val = val;
}

Value::operator bool() const
{
    return m_val == "true" ? true : false;
}

Value::operator int() const
{
    return std::atoi(m_val.c_str());
}

Value::operator double() const
{
    return std::atof(m_val.c_str());
}

Value::operator std::string() const
{
    return m_val;
}

std::string Value::val() const
{
    return m_val;
}