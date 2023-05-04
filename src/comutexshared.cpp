#include "libdco/co/comutexshared.h"

using namespace dco;

comutexshared::comutexshared(coschedule *sche)
    : state_(0), flag_(0), flag_shared_(0), wait_(sche), shared_(sche) {}

comutexshared::~comutexshared() {}

bool comutexshared::lock() {
#if DCO_MULT_THREAD
  std::unique_lock<std::mutex> lock(mtx_);
  ++flag_; // 增加占用
  int res = wait_.wait(lock, [this](coctx *) -> bool {
    if (state_ == 0) {
      --state_;
      return true;
    }
    return false;
  });
#else
  int res = wait_.wait([this](coctx *) -> bool {
    if (state_ == 0) {
      --state_;
      return true;
    }
    return false;
  });
#endif
  if (res < 0) {
    return false;
  }
  return true;
}

bool comutexshared::lock_shared() {
#if DCO_MULT_THREAD
  std::unique_lock<std::mutex> lock(mtx_);
  ++flag_shared_; // 增加占用
  int res = shared_.wait(lock, [this](coctx *) -> bool {
    if (flag_ == 0 && state_ >= 0) {
      ++state_;
      return true;
    }
    return false;
  });
#else
  int res = shared_.wait([this](coctx *) -> bool {
    if (flag_ == 0 && state_ >= 0) {
      ++state_;
      return true;
    }
    return false;
  });
#endif
  if (res < 0) {
    return false;
  }
  return true;
}

void comutexshared::unlock() {
  bool notify = false, notify_shared = false;
  {
#if DCO_MULT_THREAD
    std::lock_guard<std::mutex> lock(mtx_);
#endif
    --flag_;
    ++state_;
    notify = flag_ > 0; // 这时候state肯定是0
    notify_shared = flag_shared_ > 0;
  }
  if (notify)
    wait_.notify();
  else if (notify_shared)
    shared_.notify_all();
}

void comutexshared::unlock_shared() {
  bool notify = false;
  {
#if DCO_MULT_THREAD
    std::lock_guard<std::mutex> lock(mtx_);
#endif
    --flag_shared_;
    --state_;
    notify = flag_ > 0 && state_ == 0;
  }
  if (notify)
    wait_.notify();
}
