#pragma once
#include "libdco/co/cowaitque.h"
#include "libdco/co/cosemaphore.h"
#include "libdco/co/comutex.h"

namespace dco {

class comutexshared {
private:
  std::atomic<int> count_;
  std::atomic<int> wait_;
  cosemaphore rsem_;
  cosemaphore wsem_;
  comutex wmtx_;


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
