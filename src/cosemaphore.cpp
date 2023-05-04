#include "libdco/co/cosemaphore.h"

#include <iostream>

using namespace dco;

cosemaphore::cosemaphore(coschedule *sche, int def, int max)
    : flag_(def), max_(max), wait_(sche) {}

cosemaphore::~cosemaphore() {}

bool cosemaphore::wait_get(coctx *ctx,
                           const std::function<void(coctx *)> &call) {
  if (flag_ <= 0)
    return false;
  --flag_;
  call(ctx);
  return true;
}

bool cosemaphore::post_set() {
  if (flag_ >= max_)
    return false;
  ++flag_;
  return true;
}

int cosemaphore::wait(const std::function<void(coctx *)> &call) {
  int res = 0;
  {
#if DCO_MULT_THREAD
    std::unique_lock<std::mutex> lock(mtx_);
    res = wait_.wait(lock, [this, &call](coctx *ctx) -> bool {
      return wait_get(ctx, call);
    });
#else
    res = wait_.wait(
        [this, &call](coctx *ctx) -> bool { return wait_get(ctx, call); });
#endif
  }
  return res;
}

int cosemaphore::wait_for(time_t ms, const std::function<void(coctx *)> &call) {
  int res = 0;
  {
#if DCO_MULT_THREAD
    std::unique_lock<std::mutex> lock(mtx_);
    res = wait_.wait_for(lock, ms, [this, &call](coctx *ctx) -> bool {
      return wait_get(ctx, call);
    });
#else
    res = wait_.wait_for(
        ms, [this, &call](coctx *ctx) -> bool { return wait_get(ctx, call); });
#endif
  }
  return res;
}

int cosemaphore::wait_until(time_t ms_until,
                            const std::function<void(coctx *)> &call) {
  int res = 0;
  {
#if DCO_MULT_THREAD
    std::unique_lock<std::mutex> lock(mtx_);
    res = wait_.wait_until(lock, ms_until, [this, &call](coctx *ctx) -> bool {
      return wait_get(ctx, call);
    });
#else
    res = wait_.wait_until(ms_until, [this, &call](coctx *ctx) -> bool {
      return wait_get(ctx, call);
    });
#endif
  }
  return res;
}

int cosemaphore::notify() {
  bool ret = false;
  {
#if DCO_MULT_THREAD
    std::lock_guard<std::mutex> lock(mtx_);
    ret = post_set();
#else
    ret = post_set();
#endif
  }
  if (ret)
    return wait_.notify();
  return -1;
}
