#include "../include/error.h"

#include <string>

namespace ef
{

// ------------------------------ 外部错误注册 ------------------------------

ExternalLookupFunc& getExternalLookupFunc()
{
    static ExternalLookupFunc lookup_func = nullptr;
    return lookup_func;
}

void setExternalLookup(ExternalLookupFunc func)
{
    getExternalLookupFunc() = func;
}

const ErrorInfo* LookupExternalError(int code) noexcept
{
    return getExternalLookupFunc() ? getExternalLookupFunc()(code) : nullptr;
}

// ------------------------------ Error 构造与赋值 ------------------------------

Error::Error(int code, SourceLocation loc)
    : m_code(code), m_location(loc), m_sbo_buf{}, m_heap_buf(), m_cause(nullptr)
{
    m_sbo_buf[0] = '\0';
}

Error::Error(const Error& other) : m_code(other.m_code), m_location(other.m_location)
{
    std::memcpy(m_sbo_buf, other.m_sbo_buf, kSboSize);
    m_heap_buf = other.m_heap_buf;
    if (other.m_cause)
        m_cause = std::make_unique<Error>(*other.m_cause);
}

Error& Error::operator=(const Error& other)
{
    if (this == &other)
        return *this;

    m_code     = other.m_code;
    m_location = other.m_location;
    std::memcpy(m_sbo_buf, other.m_sbo_buf, kSboSize);
    m_heap_buf = other.m_heap_buf;
    m_cause    = other.m_cause ? std::make_unique<Error>(*other.m_cause) : nullptr;
    return *this;
}

Error::Error(Error&& other) noexcept
    : m_code(other.m_code),
      m_location(other.m_location),
      m_heap_buf(std::move(other.m_heap_buf)),
      m_cause(std::move(other.m_cause))
{
    std::memcpy(m_sbo_buf, other.m_sbo_buf, kSboSize);
}

Error& Error::operator=(Error&& other) noexcept
{
    if (this == &other)
        return *this;

    m_code     = other.m_code;
    m_location = other.m_location;
    std::memcpy(m_sbo_buf, other.m_sbo_buf, kSboSize);
    m_heap_buf = std::move(other.m_heap_buf);
    m_cause    = std::move(other.m_cause);
    return *this;
}

// ------------------------------ Error 级联 ------------------------------

Error& Error::caused_by(Error&& cause)
{
    m_cause = std::make_unique<Error>(std::move(cause));
    return *this;
}

bool Error::has_cause() const noexcept
{
    return m_cause != nullptr;
}

const Error* Error::cause() const noexcept
{
    return m_cause.get();
}

// ------------------------------ Error 访问器 ------------------------------

int Error::code() const noexcept
{
    return m_code;
}

SourceLocation Error::location() const noexcept
{
    return m_location;
}

ErrorLevel Error::level() const noexcept
{
    return LookupError(m_code)->level;
}

const char* Error::name() const noexcept
{
    return LookupError(m_code)->name;
}

const char* Error::what() const noexcept
{
    return LookupError(m_code)->message;
}

std::string Error::message() const
{
    return (m_sbo_buf[0] == '\0') ? m_heap_buf : std::string(m_sbo_buf);
}

// ------------------------------ Error 字符串格式化 ------------------------------

std::string Error::to_string() const
{
    return format(0);
}

std::string Error::format(int dept) const
{
    const ErrorInfo* info = LookupError(m_code);
    std::string      ret;

    // 级联错误添加缩进前缀
    if (dept > 0)
    {
        ret += "\n  caused by : ";
    }

    // 拼接: [Level]Name file:line message
    ret += "[" + ErrorLevel2string(info->level) + "]";
    ret += info->name;
    ret += " ";
    ret += std::string(m_location.file) + ":" + std::to_string(m_location.line);
    ret += " ";
    if (message().empty())
        ret += what();
    else
        ret += message();

    // 递归格式化级联原因
    if (m_cause)
        ret += m_cause->format(dept + 1);

    return ret;
}

}  // namespace ef