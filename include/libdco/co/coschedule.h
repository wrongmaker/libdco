#pragma once
#include <queue>
#include <thread>
#include <vector>

#include "libdco/co/coctx.h"
#include "libdco/config.h"
#include "libdco/util/cocasque.hpp"
#include "libdco/util/cominheap.hpp"
#include "libdco/util/semaphore.hpp"

/*
 * 当前调度器使用的boost.coroutines2
 * 底层协程上下文用的boost.context
 * 当前调度器与fiber调度器不同,原生多线程支持
 * 协程上下文队列由多个线程抢占,一个线程堵塞不会影响后续协程调度
 * 除非这个线程会独占一条协程的等待队列
 * 非对称协程调度
 */

namespace dco {
using namespace boost::coroutines2;

// 让调度器以无锁的方式运行在多线程环境中
class coschedule {
private:
  cocasque<coctx *> ctx_que_;
  cocasque<coctx *> ctx_buf_;
  cocasque<coctx *> ctx_timewait_add_;
  cocasque<coctx *> ctx_timewait_remove_;
  cominheap<coctx> heap_timewait_;
  std::atomic<uint32_t> ids_;
  std::atomic<bool> stop_;
#if DCO_MULT_THREAD
  semaphore sem_;
  semaphore sem_timewait_;
  std::vector<std::thread> threads_;
#endif

public:
  coschedule(const coschedule &) = delete;
#if DCO_MULT_THREAD
  coschedule(int); // 任务线程个数
#else
  coschedule();
#endif
  ~coschedule();

private:
  inline bool dco_stealing_other();
  inline void dco_stealing();
  inline time_t dco_schedule(int);
  inline time_t dco_schedule_timewait();
  inline void in_dco_add_remove(coctx *);
  inline int in_dco_resume(coctx *);
  inline int in_dco_wait(coctx *);
  inline int in_dco_sleep(coctx *);
  inline int in_dco_sleepbreak(coctx *);
  inline int in_dco_sw(coctx *);
  inline int in_dco_add(coctx *);
  inline int in_dco_gosche(coctx *);
  inline int dco_wakeup(coctx *, bool);
  inline void in_dco_add_que(coctx *);
  inline uint32_t in_dco_id_create();

public:
  coctx *dco_create(const std::function<void(coctx *)> &);
  coctx *dco_create(const std::function<void()> &);
  int dco_resume(coctx *);
  int dco_yield(
      coctx *,
      const std::function<void(coctx *)> &call = nullptr); // 让出协程,等待唤醒
  int dco_sleep(coctx *, time_t ms,
                const std::function<void(coctx *)> &call =
                    nullptr); // 毫秒级休眠 0:超时;1:打断
  int dco_sleep_until(coctx *, time_t ms_util,
                      const std::function<void(coctx *)> &call =
                          nullptr); // 毫秒级休眠 0:超时;1:打断
  int dco_gosche(coctx *);          // 让出协程,加入就绪队尾

  coctx *dco_curr_ctx(); // 获取当前执行的协程上下文
  void dco_stop();       // 停止协程

  int dco_thread_id();

#if (DCO_MULT_THREAD == 0)
#if DCO_SINGLE_LOOP_BLOCK
  void dco_main_loop(); // 单线程主循环
#else
  time_t
  dco_main_loop_noblock(); // 单线程不阻塞主循环 返回为当前调度后需要等待时间
#endif
#endif
};

} // namespace dco
