#pragma once
#include <functional>
#include <list>

namespace dco {

class codefer {
private:
  std::function<void()> func_;

public:
  void operator=(const std::function<void()> &);
  void cancel();
  void invoke();

public:
  codefer();
  codefer(const codefer &) = delete;
  codefer(const std::function<void()> &);
  ~codefer();
};

class defer_scope {
private:
  std::list<std::function<void()>> funcs_;

public:
  void operator=(const std::function<void()> &);
  void last_invoke();
  void last_cancel();

public:
  defer_scope();
  defer_scope(const defer_scope &) = delete;
  ~defer_scope();
};

} // namespace dco
