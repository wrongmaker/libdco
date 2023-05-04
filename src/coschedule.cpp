#include "libdco/co/coschedule.h"

#include <iostream>

#include "boost/assert.hpp"
#include "libdco/util/cotools.h"

using namespace dco;

thread_local coctx *local_ctx_ = nullptr;
thread_local int local_id_ = -1;
thread_local int local_steal_id_ = 0;

#if DCO_MULT_THREAD
coschedule::coschedule(int size)
    : ids_(0), stop_(false), sem_(size), sem_timewait_(1) {
  for (int i = 0; i < size; ++i)
    threads_.emplace_back(
        std::thread(std::bind(&coschedule::dco_schedule, this, i)));
  threads_.emplace_back(
      std::thread(std::bind(&coschedule::dco_schedule_timewait, this)));
}
#else
coschedule::coschedule()
    : ctx_que_(0), ctx_buf_(0), ctx_timewait_add_(0), ctx_timewait_remove_(0),
      stop_(false) {}
#endif

coschedule::~coschedule() { dco_stop(); }

bool coschedule::dco_stealing_other() {
  int size = threads_.size();
  if (size > 1) {
    if (local_steal_id_ == local_id_)
      ++local_id_;
    if (local_id_ >= size)
      local_id_ = 0;
    // 去偷
    while (size--) {
    }
    // 偷到了就开始执行
  }
  return false;
}

void coschedule::dco_stealing() {
  dco_stealing_other();
  // 去全局取完

  // 全局一个都没有说明被其他取完了
  // 既然醒着就要去再偷一遍
  dco_stealing_other();
}

time_t coschedule::dco_schedule(int id) {
  local_id_ = id;
  for (;;) { // 调度循环
    if (stop_.load())
      break; // 退出循环
#if (DCO_MULT_THREAD == 0)
    time_t timewait = dco_schedule_timewait();
#endif
    coctx *ctx = nullptr;
    // // 去本地取下

    // // 如果取不到就去偷
    // if (ctx == nullptr) {
    //   dco_stealing();
    //   // 再去本地取下
    // }
    // // 如果再取不到就等待
    // if (ctx == nullptr) {

    // }
    if (!ctx_que_.pop(ctx)) { // 获取不到上下文
#if DCO_MULT_THREAD
      sem_.try_wait();
      continue;
#else
      return timewait;
#endif
    }
    BOOST_ASSERT(ctx != nullptr);
    local_ctx_ = ctx;
    // bool run_before = ctx->run_.exchange(true);
    // BOOST_ASSERT(run_before == false);
    ctx->push_(); // 切回协程
    if (ctx->end_.load()) {
      ctx->reset();
      ctx_buf_.push(ctx); // 回收
    } else {
      // ctx->run_.store(false);
      in_dco_sw(ctx);
    }
    local_ctx_ = nullptr;
  }
  return 0;
}

time_t coschedule::dco_schedule_timewait() {
  for (;;) { // 定时器调度
    if (stop_.load())
      break; // 退出循环
    time_t timewait = -1, now = cotools::msnow();
    coctx *ctx = nullptr;
    while (ctx_timewait_add_.pop(ctx)) { // 添加
      // 只有在结果中状态改变但是没有唤醒的时候才加入超时队列
      if (0 < in_dco_add(ctx))
        heap_timewait_.push(ctx);
    }
    while (ctx_timewait_remove_.pop(ctx)) { // 移除
      heap_timewait_.remove(ctx);           // 防止索引重置
      in_dco_sleepbreak(ctx);
    }
    while (!heap_timewait_.empty()) {
      ctx = heap_timewait_.top();
      if (ctx->timeout_ <= now) {   // 有时间过期了
        heap_timewait_.remove(ctx); // 防止索引重置
        dco_wakeup(ctx, false);     // 超时了尝试唤醒
      } else {
        timewait = ctx->timeout_ - now;
        break;
      }
    }
#if DCO_MULT_THREAD
    if (timewait < 0)
      sem_timewait_.try_wait();
    else
      sem_timewait_.try_wait_for(timewait);
#else
    return timewait < 0 ? INT_MAX : timewait;
#endif
  }
  return 0;
}

void coschedule::in_dco_add_remove(coctx *ctx) {
  ctx_timewait_remove_.push(ctx);
#if DCO_MULT_THREAD
  sem_timewait_.try_notify();
#endif
}

int coschedule::in_dco_resume(coctx *ctx) {
  if (!ctx)
    return -100;
  int res = ctx->try_resume();
  return res;
}

int coschedule::in_dco_wait(coctx *ctx) {
  // if (!ctx || ctx != local_ctx_)
  if (!ctx)
    return -100;
  int res = ctx->try_yield();
  return res;
}

int coschedule::in_dco_sleep(coctx *ctx) {
  // if (!ctx || ctx != local_ctx_)
  if (!ctx)
    return -100;
  int res = ctx->try_sleep();
  return res;
}

int coschedule::in_dco_sleepbreak(coctx *ctx) {
  if (!ctx)
    return -100;
  int res = ctx->try_sleepbreak();
  if (0 != res)
    return res;
  in_dco_add_que(ctx);
  return res;
}

int coschedule::in_dco_sw(coctx *ctx) {
  if (!ctx)
    return -100;
  int res = ctx->try_sw();
  if (0 != res) {
    if (103 == res) //
      in_dco_add_remove(ctx);
    return res;
  }
  in_dco_add_que(ctx);
  return res;
}

int coschedule::in_dco_add(coctx *ctx) {
  if (!ctx)
    return -100;
  int res = ctx->try_add();
  if (0 != res)
    return res;
  in_dco_add_que(ctx);
  return res;
}

int coschedule::in_dco_gosche(coctx *ctx) {
  if (!ctx)
    return -100;
  return ctx->try_gosche();
  // int res = in_dco_wait(ctx);
  // if (0 != res)
  //   return res;
  // // 再唤醒自己(能被挂起就能被唤醒)
  // dco_resume(ctx);
  // return res;
}

int coschedule::dco_wakeup(coctx *ctx, bool bbreak) {
  int res = in_dco_resume(ctx);
  if (0 != res) {
    if (103 == res) {
      if (bbreak) { // 如果是被打断的 需要走一遍remove队列
        ctx->break_ = bbreak;
        in_dco_add_remove(ctx);
        return 0;
      } else {                         // 如果是超时唤醒的
        return in_dco_sleepbreak(ctx); // 直接给他唤醒
      }
    } else if (res == 3) {
      // == 3 说明也是被打断唤醒的 只是这里不能加入调度队列
      ctx->break_ = bbreak;
    }
    return res;
  }
  ctx->break_ = bbreak;
  in_dco_add_que(ctx);
  return res;
}

void coschedule::in_dco_add_que(coctx *ctx) {
  BOOST_ASSERT(ctx != nullptr);
  // printf("add ctx:%p\n", ctx);
  ctx_que_.push(ctx);
#if DCO_MULT_THREAD
  sem_.try_notify();
#endif
}

uint32_t coschedule::in_dco_id_create() { return ++ids_; }

coctx *coschedule::dco_create(const std::function<void(coctx *)> &call) {
  coctx *ctx = nullptr;
  if (!ctx_buf_.pop(ctx)) {
    ctx = new coctx(in_dco_id_create());
  }
  BOOST_ASSERT(ctx != nullptr);
  // ctx->reset();
  coroutine<void>::push_type push(
      [this, call, ctx](coroutine<void>::pull_type &pull) {
        ctx->pull_ = std::move(pull); // 设置再入点
        call(ctx);                    // 调用函数
        ctx->end_.store(true);        // 标记为结束
                                      // 结束后切回调度
      });
  ctx->push_ = std::move(push);
  return ctx;
}

coctx *coschedule::dco_create(const std::function<void()> &call) {
  coctx *ctx = nullptr;
  if (!ctx_buf_.pop(ctx)) {
    ctx = new coctx(in_dco_id_create());
  }
  BOOST_ASSERT(ctx != nullptr);
  // ctx->reset();
  coroutine<void>::push_type push(
      [this, call, ctx](coroutine<void>::pull_type &pull) {
        ctx->pull_ = std::move(pull); // 设置再入点
        call();                       // 调用函数
        ctx->end_.store(true);        // 标记为结束
                                      // 结束后切回调度
      });
  ctx->push_ = std::move(push);
  return ctx;
}

int coschedule::dco_resume(coctx *ctx) {
  int res = dco_wakeup(ctx, true);
  return res;
}

int coschedule::dco_yield(coctx *ctx,
                          const std::function<void(coctx *)> &call) {
  int res = in_dco_wait(ctx);
  if (0 != res)
    return res;
  if (call)
    call(ctx);
  ctx->pull_(); // 切换回调度
  return 0;
}

int coschedule::dco_sleep(coctx *ctx, time_t ms,
                          const std::function<void(coctx *)> &call) {
  return dco_sleep_until(ctx, cotools::ms2until(ms), call);
}

int coschedule::dco_sleep_until(coctx *ctx, time_t ms_until,
                                const std::function<void(coctx *)> &call) {
  int res = in_dco_sleep(ctx);
  if (0 != res)
    return res;
  if (call)
    call(ctx); // 执行事务
  // 设置超时信息
  ctx->set_timeout(ms_until);
  ctx_timewait_add_.push(ctx);
#if DCO_MULT_THREAD
  sem_timewait_.try_notify();
#endif
  ctx->pull_(); // 切换回调度
  return ctx->break_ ? 1 : 0;
}

int coschedule::dco_gosche(coctx *ctx) { // 让出协程,加入就绪队尾
  // 进行回调度器尝试
  int res = in_dco_gosche(ctx);
  if (0 != res)
    return res;
  ctx->pull_(); // 切换回调度
  return 0;
}

coctx *coschedule::dco_curr_ctx() { // 获取当前执行的协程上下文
  return local_ctx_;
}

void coschedule::dco_stop() {
  stop_.store(true);
#if DCO_MULT_THREAD
  for (size_t i = 0; i < threads_.size() - 1; ++i)
    sem_.try_notify();
  sem_timewait_.try_notify();
  for (size_t i = 0; i < threads_.size(); ++i)
    threads_[i].join();
#endif
} // 停止协程

int coschedule::dco_thread_id() { return local_id_; }

#if (DCO_MULT_THREAD == 0)
#if DCO_SINGLE_LOOP_BLOCK
void coschedule::dco_main_loop() {
  for (;;) {
    time_t timewait = dco_schedule();
    if (stop_.load())
      break;
#ifdef _WIN32
    Sleep(timewait);
#else
    usleep(timewait * 1000);
#endif
  }

} // 单线程主循环
#else
time_t coschedule::dco_main_loop_noblock() {
  return dco_schedule();
} // 单线程不阻塞主循环 返回为当前调度后需要等待时间
#endif
#endif