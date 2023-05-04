#pragma once
#include "libdco/co/cowaitque.h"

namespace dco {

class cosemaphore {
private:
  volatile int flag_; // 防止多线程环境下数据错误
  int max_;
#if DCO_MULT_THREAD
  std::mutex mtx_;
#endif
  cowaitque wait_;

public:
  cosemaphore(const cosemaphore &) = delete;
  cosemaphore(coschedule *sche, int def, int max = INT_MAX);
  virtual ~cosemaphore();

private:
  inline bool wait_get(coctx *, const std::function<void(coctx *)> &);
  inline bool post_set();

public:
  int wait(const std::function<void(coctx *)> & = [](coctx *) {
  }); // -1:失败,1:成功
  int wait_for(
      time_t ms, const std::function<void(coctx *)> & = [](coctx *) {
      }); // -1:失败,1:成功,0:超时
  int wait_until(
      time_t ms_until, const std::function<void(coctx *)> & = [](coctx *) {
      });       // -1:失败,1:成功,0:超时
  int notify(); // -1:失败,1:成功
};

} // namespace dco
