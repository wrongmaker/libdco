#pragma once
#include "libdco/co/coschedule.h"
#include "libdco/util/conolist.hpp"

namespace dco {

class comutex_base {
public:
  virtual bool lock() = 0;
  virtual bool unlock() = 0;
};

// 不保证顺序
// 性能好
class comutex : public comutex_base {
private:
  coschedule *ptr_sche_;
  std::atomic<bool> flag_; // 防止多线程环境下数据错误
  volatile coctx *ctx_curr_;
  cocasque<coctx *> ctx_que_; // 等待队列

public:
  comutex(const comutex &) = delete;
  comutex(coschedule *sche);
  virtual ~comutex();

public:
  bool lock();
  bool unlock();
};

// 保证顺序
class comutex_seq : public comutex_base {
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
  bool flag_;
  coctx *ctx_curr_;
  conolist<waitnode> ctx_que_; // 等待队列
#if DCO_MULT_THREAD
  mutable boost::detail::spinlock mtx_;
#endif

public:
  comutex_seq(const comutex_seq &) = delete;
  comutex_seq(coschedule *sche);
  virtual ~comutex_seq();

public:
  bool lock();
  bool unlock();
};

} // namespace dco
