#pragma once
#include "libdco/co/comutex.h"
#include "libdco/co/cowaitque.h"

namespace dco {

// 不保证顺序
class cocondvariable {
private:
  struct waitnode {
    waitnode *prev_;
    waitnode *next_;
    coctx *ctx_;
    waitnode(coctx *ctx) : prev_(nullptr), next_(nullptr), ctx_(ctx) {}
    waitnode() = delete;
    waitnode(const waitnode &) = delete;
  };

private:
  coschedule *ptr_sche_;
  conolist_mt<waitnode> ctx_que_; // 等待队列

public:
  cocondvariable(const cocondvariable &) = delete;
  cocondvariable(coschedule *sche);
  virtual ~cocondvariable();

public:
  int wait(std::unique_lock<comutex_base> &, const std::function<bool()> &cond);
  int wait_for(time_t ms, std::unique_lock<comutex_base> &,
               const std::function<bool()> &cond);
  int wait_until(time_t ms_until, std::unique_lock<comutex_base> &,
                 const std::function<bool()> &cond);
  bool notify();
  int notify_all();
};

} // namespace dco
