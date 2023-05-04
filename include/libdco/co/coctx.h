#pragma once
#include "boost/coroutine2/all.hpp"
#include "boost/smart_ptr/detail/spinlock.hpp"
#include "libdco/config.h"
#include <cstdint>

namespace dco {
using namespace boost::coroutines2;

template <class T> class cominheap;
class coschedule;
class coctx {
private:
  // 状态
  enum coctx_status {
    Ready,      // 就绪
    Yield,      // 等待
    Sleep,      // 休眠
    SleepBreak, // 休眠打断
  };

  // 设置
  enum coctx_flag {
    Set,
    Reset,
    Break,
  };

private:
  coroutine<void>::pull_type pull_;
  coroutine<void>::push_type push_;
  // 加锁数据 volatile 防止多线程环境数据错误
  volatile coctx_status status_;
  volatile coctx_flag sw_;
  volatile coctx_flag add_;
#if DCO_MULT_THREAD
#if DCO_CONTEXT_SPINLOCK
  mutable boost::detail::spinlock mtx_;
#else
  std::mutex mtx_;
#endif
#endif

  // 不加锁数据 非线程安全 定时器用
  volatile size_t index_;
  volatile time_t timeout_;
  uint32_t id_;
  volatile bool break_;

  std::atomic<bool> run_;
  std::atomic<bool> end_;

private:
  friend class coschedule;
  friend class cominheap<coctx>;

private:
  static void default_pull(coroutine<void>::push_type &);
  static void default_push(coroutine<void>::pull_type &);
  void set_resume();

private:
  int try_resume();
  int try_yield();
  int try_sleep();
  int try_sleepbreak();
  int try_sw();
  int try_add();
  int try_reset_resume();
  int try_gosche();
  void set_timeout(volatile time_t timeout);
  void reset();

public:
  uint32_t id() const { return id_; }

public:
  bool operator<(const coctx &obj) const { return timeout_ < obj.timeout_; }

public:
  coctx(uint32_t id);
  coctx(const coctx &) = delete;
};

} // namespace dco
