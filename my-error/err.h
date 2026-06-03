/// @brief 通用基础数据错误类型
enum class ErrorKind {
  InvalidParam,   ///< 空指针
  Full,      ///< 满了
  Empty,     ///< 空的
  OOR,       ///< 超出边界
  Overflow,  ///< 溢出，保护区已损坏
  Exist,   ///< 已被占用
  NotExist,  ///< 不存在
  TimeOut, ///< 超时
};

constexpr std::initializer_list<ErrorEntry<ErrorKind>> list = {
    {ErrorKind::InvalidParam, "invalid parameter", ErrorLevel::Avoid},
    {ErrorKind::Full, "Full", ErrorLevel::CBC},
    {ErrorKind::Empty, "Empty", ErrorLevel::CBC},
    {ErrorKind::OOR, "Out Of Range", ErrorLevel::Avoid},
    {ErrorKind::Overflow, "Overflow", ErrorLevel::Avoid},
    {ErrorKind::Exist, "already Exist", ErrorLevel::CBC},
    {ErrorKind::NotExist, "Not Exist", ErrorLevel::CBC},
    {ErrorKind::TimeOut, "TimeOut", ErrorLevel::CBC},
};
IMPL_TOSTR_LEVEL(list)

using CollectionError = Error<ErrorKind>;
template <typename T>
using CR = earth::mantle::Result<T, CollectionError>;

#define CEF(kind) CollectionError::from(kind, META)