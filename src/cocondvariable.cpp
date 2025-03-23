#include "libdco/co/cocondvariable.h"
#include "boost/smart_ptr/shared_ptr.hpp"
#include "libdco/co/all.h"

using namespace dco;

#define co_ms_now_cocondvariable                                               \
  std::chrono::duration_cast<std::chrono::milliseconds>(                       \
      std::chrono::system_clock::now().time_since_epoch())                     \
      .count()

cocondvariable::cocondvariable(coschedule *sche) : ptr_sche_(sche) {}

cocondvariable::~cocondvariable() {
  BOOST_ASSERT(!ctx_que_.pop(nullptr)); // 这时候应该是没有东西的
}

int cocondvariable::wait(std::unique_lock<comutex_base> &lock,
                         const std::function<bool()> &cond) {
  coctx *ctx = ptr_sche_->dco_curr_ctx();
  waitnode node(ctx);
  while (!cond()) {
    if (0 != ptr_sche_->dco_yield(ctx, [this, &node, &cond, &lock](coctx *ctx) {
          // 加入队列
          ctx_que_.push(&node);
          // 等加入在解锁 防止略过唤醒
          lock.mutex()->unlock(); // unlock
        })) {
      // await fail un call callback
      // do not need relock
      BOOST_ASSERT(false);
      return -1;
    }
    lock.mutex()->lock(); // relock
  }
  return 1; // 被打断了或者条件为真
}

int cocondvariable::wait_for(time_t ms, std::unique_lock<comutex_base> &lock,
                             const std::function<bool()> &cond) {
  time_t ms_until = ms + co_ms_now_cocondvariable;
  return wait_until(ms_until, lock, cond);
}

int cocondvariable::wait_until(time_t ms_until,
                               std::unique_lock<comutex_base> &lock,
                               const std::function<bool()> &cond) {
  coctx *ctx = ptr_sche_->dco_curr_ctx();
  waitnode node(ctx);
  while (!cond()) {
    int sleep_res = ptr_sche_->dco_sleep_until(
        ctx, ms_until, [this, &node, &cond, &lock](coctx *ctx) {
          // 加入队列
          ctx_que_.push(&node);
          // 等加入在解锁 防止略过唤醒
          lock.mutex()->unlock(); // unlock
        });
    if (0 > sleep_res) {
      // await fail un call callback
      // do not need relock
      BOOST_ASSERT(false);
      return -1; // 挂起失败
    }
    if (0 == sleep_res) {
      // 移除超时
      ctx_que_.remove(&node);
      // await success need relock
      lock.mutex()->lock();
      return 0; // 超时
    }
    lock.mutex()->lock(); // relock
  }
  return 1; // 被打断了或者条件为真
}

bool cocondvariable::notify() {
  return dco_futex::dco_notify(ptr_sche_, ctx_que_) == 1;
}

int cocondvariable::notify_all() {
  int times = 0;
  for (;;) {
    if (!notify())
      break;
    ++times;
  }
  return times;
}

// 这个没有close操作 不关心加入顺序