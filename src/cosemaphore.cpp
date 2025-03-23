#include "libdco/co/cosemaphore.h"

#include "libdco/co/all.h"
#include "libdco/util/cotools.h"

using namespace dco;

cosemaphore::cosemaphore(coschedule *sche, int def)
    : ptr_sche_(sche), sem_(def) {}

cosemaphore::~cosemaphore() {}

bool cosemaphore::wait() {
  coctx *ctx = ptr_sche_->dco_curr_ctx();
  BOOST_ASSERT(ctx != nullptr);
  while (!cotools::atomic_decrement_if_positive(sem_, 0, 1))
    dco_futex::dco_wait(ptr_sche_, ctx_que_, sem_);
  return true;
}

void cosemaphore::notify() {
  // 新增值
  ++sem_;
  // 开始唤醒
  dco_futex::dco_notify(ptr_sche_, ctx_que_);
}

cosemaphore_tm::cosemaphore_tm(coschedule *sche, int def)
    : sem_(def), ptr_sche_(sche) {}

cosemaphore_tm::~cosemaphore_tm() {}

int cosemaphore_tm::wait() {
  coctx *ctx = ptr_sche_->dco_curr_ctx();
  BOOST_ASSERT(ctx != nullptr);
  waitnode node(ctx);
  while (!cotools::atomic_decrement_if_positive(sem_, 0, 1))
    dco_futex::dco_wait(ptr_sche_, ctx_que_, node, sem_);
  return 1; // 被打断了或者条件为真
}

int cosemaphore_tm::wait_for(time_t ms) {
  time_t ms_until = cotools::ms2until(ms);
  return wait_until(ms_until);
}

int cosemaphore_tm::wait_until(time_t ms_until) {
  coctx *ctx = ptr_sche_->dco_curr_ctx();
  BOOST_ASSERT(ctx != nullptr);
  waitnode node(ctx);
  while (!cotools::atomic_decrement_if_positive(sem_, 0, 1)) {
    int res =
        dco_futex::dco_wait_until(ptr_sche_, ms_until, ctx_que_, node, sem_);
    if (0 == res)
      return 0;
  }
  return 1; // 被打断了或者条件为真
}

int cosemaphore_tm::notify() {
  ++sem_;
  // 尝试唤醒一个
  return dco_futex::dco_notify(ptr_sche_, ctx_que_);
}
