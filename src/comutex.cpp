#include "libdco/co/comutex.h"

#include "libdco/co/all.h"

using namespace dco;

comutex::comutex(coschedule *sche)
    : ptr_sche_(sche), flag_(false), ctx_curr_(nullptr) {}

comutex::~comutex() {
  coctx *ctx = nullptr;
  // 这时候需要等待队列中没有任何的协程在
  BOOST_ASSERT(ctx_que_.pop(ctx) == false);
}

bool comutex::lock() {
  coctx *ctx = ptr_sche_->dco_curr_ctx();
  BOOST_ASSERT(ctx != nullptr);
  // 如果无法获取锁 那就挂起尝试 true表示无法获取锁
  while (flag_.exchange(true))
    dco_futex::dco_wait(ptr_sche_, ctx_que_, flag_);
  // 设置锁协程
  ctx_curr_ = ctx;
  return true; // 获取到了锁
}

bool comutex::unlock() {
  BOOST_ASSERT(ptr_sche_->dco_curr_ctx() != nullptr &&
               ctx_curr_ == ptr_sche_->dco_curr_ctx() && flag_.load() == true);
  if (ctx_curr_ != ptr_sche_->dco_curr_ctx())
    return false;
  if (!flag_.exchange(false)) // 如果是false说明没锁定过
    return false;
  dco_futex::dco_notify(ptr_sche_, ctx_que_);
  return true;
}
