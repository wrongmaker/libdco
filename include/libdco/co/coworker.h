#pragma once

#include "coctx.h"

#include "libdco/util/cocasque.hpp"
#include "libdco/util/cominheap.hpp"
#include "libdco/util/semaphore.hpp"
#include <atomic>

namespace dco {

class coschedule;
class coworker {
private:
  friend class coschedule;

private:
  coschedule *sche_;
  int tid_;
  cocasque<coctx *> time_remove_;
  cominheap<coctx> time_wait_;
  semaphore sem_;
  int work_count_ = 0;

  // 交付调度器使用
  std::atomic<bool> run_;
  std::atomic<bool> wait_;

public:
  coworker(coschedule *sche, int tid);

protected:
  coctx *dco_get_next();
  void dco_sleep_add(coctx *);
  void dco_sleep_resume(coctx *);
  inline time_t dco_time_proc();
  inline void dco_sleep_remove(coctx *);
  inline void dco_sleep_timeout(coctx *);

public:
  int dco_work_id();
  int dco_work_count();
};

} // namespace dco
