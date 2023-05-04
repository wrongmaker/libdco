#include "libdco/util/codefer.h"

using namespace dco;

void codefer::operator=(const std::function<void()> &func) {
  if (func_)
    func_();
  func_ = std::forward<const std::function<void()> &>(func);
}

void codefer::cancel() { func_ = nullptr; }

void codefer::invoke() {
  if (func_)
    func_();
  func_ = nullptr;
}

codefer::codefer() : func_(nullptr) {}

codefer::codefer(const std::function<void()> &func) : func_(func) {}

codefer::~codefer() {
  if (func_)
    func_();
}

void defer_scope::operator=(const std::function<void()> &func) {
  funcs_.emplace_front(func);
}

void defer_scope::last_cancel() {
  if (!funcs_.empty()) {
    funcs_.pop_front();
  }
}

void defer_scope::last_invoke() {
  if (!funcs_.empty()) {
    funcs_.front()();
    funcs_.pop_front();
  }
}

defer_scope::defer_scope() {}

defer_scope::~defer_scope() {
  for (auto &func : funcs_) {
    func();
  }
}