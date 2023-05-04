#pragma once
#include "libdco/co/cowaitque.h"

namespace dco {

class comutexshared {
private:
  int state_;       // 占用
  int flag_;        //
  int flag_shared_; //
  cowaitque wait_;
  cowaitque shared_;
#if DCO_MULT_THREAD
  std::mutex mtx_;
#endif

public:
  comutexshared(const comutexshared &) = delete;
  comutexshared(coschedule *sche);
  virtual ~comutexshared();

public:
  bool lock();
  bool lock_shared();
  void unlock();
  void unlock_shared();
};

} // namespace dco
