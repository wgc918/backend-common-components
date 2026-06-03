//一个具体模块中的错误类型定义

/// @brief 通用基础数据错误类型
enum class ErrorKind {
  InvalidParam,   ///< 空指针
  NotMatch,  ///< 不匹配
  OpenFileFail,
  IndentationNotMatch,
  Database,
  NotSupport,
};

constexpr std::initializer_list<ErrorEntry<ErrorKind>> list = {
    {ErrorKind::InvalidParam, "Invalid param", ErrorLevel::Avoid},
    {ErrorKind::NotMatch, "Not Match", ErrorLevel::Avoid},
    {ErrorKind::OpenFileFail, "Cannot open file", ErrorLevel::Avoid},
    {ErrorKind::IndentationNotMatch, "Space indentation is less than 2", ErrorLevel::Avoid},
    {ErrorKind::Database, "Database error", ErrorLevel::Avoid},
    {ErrorKind::NotSupport, "NotSupport", ErrorLevel::Avoid},
};
IMPL_TOSTR_LEVEL(list)

constexpr std::initializer_list<
    ErrorKindPair<serde::ErrorKind, collection::ErrorKind>>
    collection2serde = {
                 {ErrorKind::InvalidParam, collection::ErrorKind::InvalidParam},
                 {ErrorKind::InvalidParam, collection::ErrorKind::Full},
                 {ErrorKind::InvalidParam, collection::ErrorKind::Empty},
                 {ErrorKind::InvalidParam, collection::ErrorKind::TimeOut},
                 {ErrorKind::InvalidParam, collection::ErrorKind::OOR},
                 {ErrorKind::InvalidParam, collection::ErrorKind::Overflow},
                 {ErrorKind::InvalidParam, collection::ErrorKind::Exist},
                 {ErrorKind::InvalidParam, collection::ErrorKind::NotExist}
    };
BUILD_ERR_MAP(collection2serde, collection::list)

constexpr std::initializer_list<
    ErrorKindPair<serde::ErrorKind, system::ErrorKind>>
    system2serde = {
        {ErrorKind::InvalidParam, system::ErrorKind::InvalidParam},
        {ErrorKind::Database, system::ErrorKind::Full},
        {ErrorKind::Database, system::ErrorKind::Empty},
        {ErrorKind::Database, system::ErrorKind::Internal},
        {ErrorKind::OpenFileFail, system::ErrorKind::OpenCloseFail},
        {ErrorKind::Database, system::ErrorKind::MapUnmapFail},
        {ErrorKind::Database, system::ErrorKind::ReadWriteFail},
        {ErrorKind::NotSupport, system::ErrorKind::Unsupported},
      };
BUILD_ERR_MAP(system2serde, system::list)

using SerdeError = Error<ErrorKind>;
template <typename T>
using SR = earth::mantle::Result<T, SerdeError>;

#define SDEF(kind) SerdeError::from(kind, META)