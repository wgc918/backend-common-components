/// @brief 错误级别
enum class ErrorLevel {
  Fatal,  ///< 致命错误
  CBC,    ///< 需要Case by Case 处理
  Avoid,  ///< 可以做防范处理的
};

/// @brief 根据level给出建议提示string
/// @param level
/// @return Error建议string
Str suggestion(ErrorLevel level);

template <typename DstEK, typename SrcEK>
struct ErrorKindPair {
  using DstType = DstEK;
  using SrcType = SrcEK;
  DstEK dst;
  SrcEK src;
};

template <typename DstEK, typename SrcEK>
DstEK from_ek(SrcEK err,
              const std::initializer_list<ErrorKindPair<DstEK, SrcEK>>& list) {
  for (const ErrorKindPair<DstEK, SrcEK> entry : list) {
    if (err == entry.src) {
      return entry.dst;
    }
  }
  mustNotExecute();
}

/// @brief 表示错误的种类，等级和文本信息
template <typename ErrorKind>
struct ErrorEntry {
  ErrorKind kind;
  const char* str;
  ErrorLevel level;
};

void printStackTrace();

#define META ErrorMeta({LOCATION})

/// @brief enum与字符串的互相转换
template <typename EnumKind>
struct EnumStrEntry {
  EnumKind en;
  const char* str;
};

template <typename EnumKind>
EnumKind strToEnum(const char* str);

template <typename EnumKind>
bool isValidEnum(const char* str);

#define DEFINE_ENUM_STR_MACRO(Enum, list)        \
    inline const char* enumToStr(Enum en) {      \
    for (auto& entry : list) {                   \
      if (en == entry.en) {                      \
        return entry.str;                        \
      }                                          \
    }                                            \
    mustNotExecute("TempEnum not exit");         \
  }                                              \
  template <>                                    \
  inline Enum strToEnum(const char* str) {       \
    for (auto& entry : list) {                   \
      if (earth::mantle::Str(str) ==             \
          earth::mantle::Str(entry.str)) {       \
        return entry.en;                         \
      }                                          \
    }                                            \
    mustNotExecute("no matching TempEnum");      \
  }                                              \
  template <>                                    \
  inline bool isValidEnum<Enum>(const char* str) {     \
    for (auto& entry : list) {                   \
      if (earth::mantle::Str(str) ==             \
          earth::mantle::Str(entry.str)) {       \
        return true;                             \
      }                                          \
    }                                            \
    return false;                                \
  }
}  // namespace mantle
}  // namespace earth

#define IMPL_TOSTR_LEVEL(list)                                            \
  template <typename ErrorKind>                                           \
  earth::mantle::Str to_str(ErrorKind kind) {                       \
    for (const earth::mantle::ErrorEntry<ErrorKind> entry : list) { \
      if (kind == entry.kind) {                                           \
        return earth::mantle::Str(entry.str);                       \
      }                                                                   \
    }                                                                     \
    mustNotExecute("Errorkind not exit");                                 \
  }                                                                       \
  template <typename ErrorKind>                                           \
  earth::mantle::ErrorLevel level(ErrorKind kind) {                 \
    for (const earth::mantle::ErrorEntry<ErrorKind> entry : list) { \
      if (kind == entry.kind) {                                           \
        return entry.level;                                               \
      }                                                                   \
    }                                                                     \
    mustNotExecute("Errorkind not exit");                                 \
  }

namespace earth {
namespace mantle {
#define NEED_CHECK_IS_OK

#ifdef NEED_CHECK_IS_OK
#define MANTLE_RESULT_FALSE false
#else
#define MANTLE_RESULT_FALSE true
#endif

/// @brief to string
/// @param str
/// @return
inline ::std::string to_str(const ::std::string& str) { return {str}; }
/// @brief Error数据结构, 包含错误类型, 出错是trace信息
/// @tparam ErrorKind 错误类型
/// @tparam NUM  最大可记录ErrorMeta信息个数
template <typename ErrorKind, size_t NUM = 7>
struct Error {
  /// @brief 根据ErrorKind及meta构建Error
  /// @param kind 错误类型
  /// @param meta 错误Meta
  /// @return Error
  using Kind = ErrorKind;
  static Error<ErrorKind> from(ErrorKind kind, ErrorMeta meta) {
    Error<ErrorKind> ret;
    ret.kind = kind;
    ret.metas[0] = meta;
    ret.len = 1U;

    return ret;
  }
  /// @brief 添加一个ErrorMeta
  /// @param meta 错误Meta信息
  void push_meta(ErrorMeta meta) {
    auto len = this->len;
    if (len < NUM) {
      this->metas[len] = meta;
      this->len++;
    }
  }
  size_t len = 0U;        ///< ErrorMeta 个数
  ErrorKind kind;        ///< 错误类型
  ErrorMeta metas[NUM];  ///< 错误meta 信息
};

template <typename DstEK, typename SrcEK>
Error<DstEK> from_error(
    const Error<SrcEK>& err, ErrorMeta meta,
    const std::initializer_list<ErrorKindPair<DstEK, SrcEK>>& list) {
  auto dst_kind = from_ek<DstEK, SrcEK>(err.kind, list);
  auto ret = Error<DstEK>::from(dst_kind, err.metas[0]);
  for (size_t i = 1; i < err.len; i++) {
    ret.push_meta(err.metas[i]);
  }
  ret.push_meta(meta);
  return ret;
}

/// @brief Error
/// @tparam ErrorKind
/// @tparam NUM
/// @param err
/// @return
template <typename ErrorKind, size_t NUM = 7>
Str to_str(const Error<ErrorKind, NUM>& err) {
  auto kind = err.kind;
  Str ret = to_str(kind) + ": " + suggestion(level(kind));
  for (size_t i = 0; i < err.len; i++) {
    ret += "\n\t\t";
    ret += to_str(err.metas[i]);
  }

  return ret;
}

enum class ResultType {
  Ok,
  Control,
  Err,
};

/// @brief 函数返回值封装, 成功, 保存具体的返回值; 保存具体的错误码
/// @tparam G 成功时, 函数返回值类型
/// @tparam E 失败时, 函数返回的错误类型
template <typename G, typename E>
struct Result {
 private:
  constexpr Result() = default;
  ResultType rt;
  mutable bool ready{MANTLE_RESULT_FALSE};
  template <typename T, typename U>
  struct ResultAndError {
    T good;
    U err;
    FlowControl ctl;
  };
  template <typename U>
  struct ErrorOnly {
    U err;
    FlowControl ctl;
  };
  using Container =
      typename std::conditional_t<std::is_void<G>::value, ErrorOnly<E>,
                                  ResultAndError<G, E>>;
  Container f;

 public:
  using Good = G;
  using ErrorKind = E;
  /// @brief 构造函数调用成功返回值为g Result对象
  /// @tparam U 返回值类型
  /// @param g 返回值
  /// @return 返回Result对象
  template <typename U = G,
            typename std::enable_if_t<!std::is_void<U>::value, int> = 0>
  constexpr static Result<G, E> Ok(U&& g) {
    auto ret = Result<G, E>();
    ret.rt = ResultType::Ok;
    ret.f.good = std::forward<U>(g);

    return ret;
  }

  /// @brief 构造函数调用成功返回值为void的Result对象
  /// @tparam U 返回值类型, 必须是void
  /// @return Result对象
  template <typename U = G,
            typename std::enable_if_t<std::is_void<U>::value, int> = 0>
  constexpr static Result<U, E> Ok() {
    auto ret = Result<U, E>();
    ret.rt = ResultType::Ok;
    return ret;
  }

  /// @brief 构造函数调用出错,错误码为e的Result对象
  /// @param e 错误码
  /// @return Result对象
  constexpr static Result<G, E> Err(E e) {
    auto ret = Result<G, E>();
    ret.rt = ResultType::Err;
    ret.f.err = e;

    return ret;
  }

  constexpr static Result<G, E> Control(FlowControl fc) {
    auto ret = Result<G, E>();
    ret.rt = ResultType::Control;
    ret.f.ctl = fc;

    return ret;
  }

  /// @brief 输出错误信息, just for debug, don't use it in lib
  /// @param msg
  void message(const char* msg) {
    if (this->is_err()) {
      auto err = this->f.err;

      if (msg != nullptr) {
#if defined(__ANDROID__)
        __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "[Mantle] %s: %s", msg,
                            to_str(err).c_str());
#else
        printf("[Mantle] %s: %s\n", msg, to_str(err).c_str());
#endif
      }
      exit(0);
    } else {
      mustNotExecute("result is_ok");
    }
  }

  /// @brief 判定Result类型是否为Ok的
  /// @return 是Ok: 返回true, 否则返回false
  constexpr bool is_ok() const {
    this->ready = true;
    return this->rt == ResultType::Ok ? true : false;
  }

  /// @brief 获取ok的值, 如果非ok, 则报错
  /// @return 返回Ok是的值
  template <typename U = G, typename std::enable_if_t<
                                std::is_copy_assignable<U>::value, int> = 0>
  constexpr G ok() const {
    if (this->ready && this->is_ok()) {
      return this->f.good;
    } else {
      if (!this->ready) {
#if defined(__ANDROID__)
        __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG,
                            "please call is_ok() first");
#else
        fprintf(stderr, "please call is_ok() first\n");
#endif
      } else {
#if defined(__ANDROID__)
        __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG,
                            "ok() is called on an error Result %s, aborting",
                            to_str(this->f.err).c_str());
#else
        fprintf(stderr, "ok() is called on an error Result %s, aborting\n",
                to_str(this->f.err).c_str());
#endif
      }
      printStackTrace();
#if defined(__ANDROID__)
#else
      abort();
#endif
    }
  }

  constexpr G ok() {
    if (this->ready && this->is_ok()) {
      return this->f.good;
    } else {
      if (!this->ready) {
#if defined(__ANDROID__)
        __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG,
                            "please call is_ok() first");
#else
        fprintf(stderr, "please call is_ok() first\n");
#endif
      } else {
#if defined(__ANDROID__)
        __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG,
                            "ok() is called on an error Result %s, aborting",
                            to_str(this->f.err).c_str());
#else
        fprintf(stderr, "ok() is called on an error Result %s, aborting\n",
                to_str(this->f.err).c_str());
#endif
      }
      printStackTrace();
#if defined(__ANDROID__)
#else
      abort();
#endif
    }
  }

  /// @brief 类似ok(), 但是使用移动语义
  /// @return 返回Ok是的值
  G move() {
    if (this->ready && this->is_ok()) {
      return std::move(this->f.good);
    } else {
      if (!this->ready) {
#if defined(__ANDROID__)
        __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG,
                            "please call is_ok() first");
#else
        fprintf(stderr, "please call is_ok() first\n");
#endif
      } else {
#if defined(__ANDROID__)
        __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG,
                            "move() is called on an error Result %s, aborting",
                            to_str(this->f.err).c_str());
#else
        fprintf(stderr, "move() is called on an error Result %s, aborting\n",
                to_str(this->f.err).c_str());
#endif
      }
      printStackTrace();
      abort();
    }
  }
  /// @brief 判定是是否为error类型
  /// @return error类型,返回true, 否则返回false
  constexpr bool is_err() const {
    this->ready = true;
    return !(this->is_ok() || this->is_control());
  }

  /// @brief 获取出错时的错误值
  /// @return 错误值
  constexpr E err() const {
    if (this->ready && this->is_err()) {
      return this->f.err;
    } else {
      if (!this->ready) {
#if defined(__ANDROID__)
        __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG,
                            "please call is_err() first");
#else
        fprintf(stderr, "please call is_err() first\n");
#endif
      } else {
#if defined(__ANDROID__)
        __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG,
                            "err() is called on an ok Result %s, aborting",
                            to_str(this->f.err).c_str());
#else
        fprintf(stderr, "err() is called on an ok Result %s, aborting\n",
                to_str(this->f.err).c_str());
#endif
      }
      printStackTrace();
      abort();
    }
  }

  constexpr E err() {
    if (this->ready && this->is_err()) {
      return this->f.err;
    } else {
      if (!this->ready) {
#if defined(__ANDROID__)
        __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG,
                            "please call is_err() first");
#else
        fprintf(stderr, "please call is_err() first\n");
#endif
      } else {
#if defined(__ANDROID__)
        __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG,
                            "err() is called on an ok Result %s, aborting\n",
                            to_str(this->f.err).c_str());
#else
        fprintf(stderr, "err() is called on an ok Result %s, aborting\n",
                to_str(this->f.err).c_str());
#endif
      }
      printStackTrace();
      abort();
    }
  }

  constexpr bool is_control() const {
    return this->rt == ResultType::Control ? true : false;
  }

  constexpr FlowControl control() {
      if(this->is_control()) {
          return this->f.ctl;
      } else {
          mustNotExecute();
      }
  }
};