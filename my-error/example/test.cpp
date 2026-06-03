//具体的使用

template <typename... Args>
SR<void> PreparedStatement::update(Args&&... args) {
  auto bind_ret = bindParams(1, std::forward<Args>(args)...);
  CHECK_AND_RETURN(bind_ret);
  return this->update();
}

template <typename... Args>
SR<void> PreparedStatement::bind(Args&&... args) {
  using R = SR<void>;
  auto bind_ret = bindParams(1, std::forward<Args>(args)...);
  CHECK_AND_RETURN(bind_ret);
  return R::Ok();
}

inline SR<void> PreparedStatement::update() {
  using R = SR<void>;
  auto ret = sqlite3_step(stmt);
  if (ret != SQLITE_DONE && ret != SQLITE_ROW) {
    __MLOGE__("Fail to update: {}", this->errorMsg());
    return R::Err(SDEF(ErrorKind::Database));
  }
  return this->reset();
}