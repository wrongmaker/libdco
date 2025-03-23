#pragma once
#include "libdco/co/coschedule.h"
#include "libdco/util/cocasque.hpp"
#include "libdco/util/conolist.hpp"

namespace dco {

class cosemaphore {
private:
  coschedule *ptr_sche_;
  cocasque<coctx *> ctx_que_; // 等待队列
  std::atomic<int> sem_;

public:
  cosemaphore(const cosemaphore &) = delete;
  cosemaphore(coschedule *sche, int def = 0);
  virtual ~cosemaphore();

public:
  bool wait();   // -1:失败,1:成功
  void notify(); // -1:失败,1:成功
};

class cosemaphore_tm {
public:
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
  std::atomic<int> sem_;
  conolist_mt<waitnode> ctx_que_; // 等待队列

public:
  cosemaphore_tm(const cosemaphore_tm &) = delete;
  cosemaphore_tm(coschedule *sche, int def);
  virtual ~cosemaphore_tm();

private:
  int try_call();

public:
  int wait();                      // -1:失败,1:成功
  int wait_for(time_t ms);         // -1:失败,1:成功,0:超时
  int wait_until(time_t ms_until); // -1:失败,1:成功,0:超时
  int notify();                    // -1:失败,1:成功
};

} // namespace dco
