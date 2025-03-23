#pragma once
#include <thread>
#include <vector>

#include "libdco/co/coctx.h"
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

class coworker;
// 让调度器以无锁的方式运行在多线程环境中
class coschedule {
protected:
  friend class coworker;

private:
  // 调度逻辑
  cocasque<coctx *> ctx_que_;
  cocasque<coctx *> ctx_buf_;
  std::atomic<uint32_t> ids_;

  // 等待工作的线程
  std::atomic<int> sem_;
  cocasque<int> worker_wait_;

  // 工作线程
  std::vector<coworker *> workers_;
  std::vector<std::thread> threads_;

public:
  coschedule(const coschedule &) = delete;
  coschedule(int); // 任务线程个数
  ~coschedule();

private:
  // 加入执行线程并尝试唤醒
  void in_dco_add_que(coctx *);

  // 管理worker
  void in_dco_wait_worker(int tid);
  inline void in_dco_call_worker();

  // 内部逻辑
  inline int in_dco_sw(coctx *);
  inline time_t in_dco_schedule(int);
  inline uint32_t in_dco_id_create();

public:
  coctx *dco_create(const std::function<void(coctx *)> &);
  coctx *dco_create(const std::function<void()> &);
  int dco_resume(coctx *);
  // 让出协程,等待唤醒
  int dco_yield(coctx *, const std::function<void(coctx *)> &call = nullptr);
  // 毫秒级休眠 0:超时;1:打断
  int dco_sleep(coctx *, time_t ms,
                const std::function<void(coctx *)> &call = nullptr);
  // 毫秒级休眠 0:超时;1:打断
  int dco_sleep_until(coctx *, time_t ms_util,
                      const std::function<void(coctx *)> &call = nullptr);
  // 让出协程,加入就绪队尾
  int dco_gosche(coctx *);
  // 获取当前执行的协程上下文
  coctx *dco_curr_ctx();

  // 当前任务线程id
  int dco_thread_id();
  // 打赢各个线程运行数
  void dco_show_worker_info();
};

} // namespace dco
