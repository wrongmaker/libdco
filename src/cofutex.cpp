#include "libdco/co/cofutex.h"

using namespace dco;

int dco_futex::dco_wait(coschedule *ptr_sche, cocasque<coctx *> &que,
                        const std::function<bool()> &check) {
  coctx *ctx = ptr_sche->dco_curr_ctx();
  BOOST_ASSERT(ctx != nullptr);
  if (0 != ptr_sche->dco_yield(ctx, [ptr_sche, &que, check](coctx *ctx) {
        if (check()) {
          ptr_sche->dco_resume(ctx);
          return;
        }
        que.push(ctx);
        if (check())
          dco_notify(ptr_sche, que);
      })) {
    BOOST_ASSERT(false); // 通知加锁失败
    return -1;
  }
  return 1; // 被打断了或者条件为真
}

int dco_futex::dco_wait(coschedule *ptr_sche, cocasque<coctx *> &que,
                        std::atomic<bool> &flag) {
  coctx *ctx = ptr_sche->dco_curr_ctx();
  BOOST_ASSERT(ctx != nullptr);
  if (0 != ptr_sche->dco_yield(ctx, [ptr_sche, &que, &flag](coctx *ctx) {
        if (!flag.load()) {
          ptr_sche->dco_resume(ctx);
          return;
        }
        que.push(ctx);
        if (!flag.load())
          dco_notify(ptr_sche, que);
      })) {
    BOOST_ASSERT(false); // 通知加锁失败
    return -1;
  }
  return 1; // 被打断了或者条件为真
}

int dco_futex::dco_wait(coschedule *ptr_sche, cocasque<coctx *> &que,
                        std::atomic<int> &sem) {
  coctx *ctx = ptr_sche->dco_curr_ctx();
  BOOST_ASSERT(ctx != nullptr);
  if (0 != ptr_sche->dco_yield(ctx, [ptr_sche, &que, &sem](coctx *ctx) {
        if (sem.load() > 0) {
          ptr_sche->dco_resume(ctx);
          return;
        }
        que.push(ctx);
        if (sem.load() > 0)
          dco_notify(ptr_sche, que);
      })) {
    BOOST_ASSERT(false); // 通知加锁失败
    return -1;
  }
  return 1; // 被打断了或者条件为真
}

int dco_futex::dco_notify(coschedule *ptr_sche, cocasque<coctx *> &que) {
  coctx *ctx = nullptr;
  if (!que.pop(ctx))
    return -1;
  // 可能因为超时等各种原因唤醒失败，只能用来判断是否唤醒成功，后续不能在使用ctx这个节点操作，可能已经调度了
  return ptr_sche->dco_resume(ctx) >= 0 ? 1 : -1;
}

void dco_futex::dco_notify_all(coschedule *ptr_sche, cocasque<coctx *> &que) {
  coctx *ctx = nullptr;
  while (que.pop(ctx))
    ptr_sche->dco_resume(ctx);
}
