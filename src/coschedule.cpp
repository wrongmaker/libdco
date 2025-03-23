#include "libdco/co/coschedule.h"

#include "boost/assert.hpp"
#include "libdco/co/coctx.h"
#include "libdco/co/coworker.h"
#include "libdco/util/cotools.h"

using namespace dco;

thread_local coctx *local_ctx_ = nullptr;
thread_local coworker *local_worker_ = nullptr;
thread_local int local_id_ = -1;
thread_local int local_steal_id_ = 0;

coschedule::coschedule(int size) : ids_(0) {
  workers_.resize(size);
  threads_.resize(size);
  for (int i = 0; i < size; ++i) {
    workers_[i] = new coworker(this, i);
    worker_wait_.push(i);
  }
  for (int i = 0; i < size; ++i)
    threads_[i] = std::move(
        std::thread(std::bind(&coschedule::in_dco_schedule, this, i)));
}

coschedule::~coschedule() {}

void coschedule::in_dco_add_que(coctx *ctx) {
  BOOST_ASSERT(ctx != nullptr);
  // 塞入等待队列
  ctx_que_.push(ctx);
  in_dco_call_worker();
}

void coschedule::in_dco_wait_worker(int tid) {
  // 如果原来不在等待才可以加入
  if (workers_[tid]->wait_.exchange(true))
    return;
  // 加入到等待队列中
  // printf("wait tid:%d\n", tid);
  workers_[tid]->run_.exchange(false);
  worker_wait_.push(tid);
  // 加入成功，如果有等待唤醒的就尝试唤醒一下
  if (cotools::atomic_decrement_if_positive(sem_, 0, 1))
    in_dco_call_worker();
}

void coschedule::in_dco_call_worker() {
  for (;;) {
    int tid;
    if (!worker_wait_.pop(tid)) {
      // 没有可以唤醒的 增加一个sem
      cotools::atomic_increase_if_positive(sem_, workers_.size(), 1);
      break;
    }
    // 如果原来非运行中，就把他唤醒
    if (!workers_[tid]->run_.exchange(true)) {
      // printf("call tid:%d\n", tid);
      workers_[tid]->wait_.exchange(false);
      workers_[tid]->sem_.try_notify();
      break;
    }
  }
}

int coschedule::in_dco_sw(coctx *ctx) {
  BOOST_ASSERT(ctx != nullptr);
  int res = ctx->try_sw_full();
  if (res < 0)
    return res;
  if (res == 0)
    in_dco_add_que(ctx);
  else if (res == 2)
    local_worker_->dco_sleep_add(ctx);
  return res;
}

time_t coschedule::in_dco_schedule(int id) {
  local_id_ = id;
  local_worker_ = workers_[id];
  for (;;) { // 调度循环
    // 尝试进行协程获取
    coctx *ctx = local_worker_->dco_get_next();
    if (ctx == nullptr)
      continue;
    BOOST_ASSERT(ctx != nullptr);
    BOOST_ASSERT(ctx->get_status() == coctx::Ready_Reset_Reset);
    local_ctx_ = ctx;
    ctx->tid_ = local_id_; // 标记线程id
    ctx->swin_();          // 切回协程
    if (ctx->end_) {
      ctx->reset();
      ctx_buf_.push(ctx); // 回收
    } else {
      BOOST_ASSERT(ctx->tid_ == local_id_);
      in_dco_sw(ctx);
    }
    local_ctx_ = nullptr;
  }
  return 0;
}

uint32_t coschedule::in_dco_id_create() { return ++ids_; }

coctx *coschedule::dco_create(const std::function<void(coctx *)> &call) {
  coctx *ctx = nullptr;
  if (!ctx_buf_.pop(ctx)) {
    ctx = new coctx(in_dco_id_create());
  }
  BOOST_ASSERT(ctx != nullptr);
  coroutine<void>::push_type push(
      [this, call, ctx](coroutine<void>::pull_type &pull) {
        ctx->swout_ = std::move(pull); // 设置再入点
        call(ctx);                     // 调用函数
        ctx->end_ = true;              // 标记为结束
      });
  ctx->swin_ = std::move(push);
  return ctx;
}

coctx *coschedule::dco_create(const std::function<void()> &call) {
  coctx *ctx = nullptr;
  if (!ctx_buf_.pop(ctx)) {
    ctx = new coctx(in_dco_id_create());
  }
  BOOST_ASSERT(ctx != nullptr);
  coroutine<void>::push_type push(
      [this, call, ctx](coroutine<void>::pull_type &pull) {
        ctx->swout_ = std::move(pull); // 设置再入点
        call();                        // 调用函数
        ctx->end_ = true;              // 标记为结束
      });
  ctx->swin_ = std::move(push);
  return ctx;
}

int coschedule::dco_resume(coctx *ctx) {
  if (!ctx)
    return -100;
  int res = ctx->try_resume_full();
  if (res < 0)
    return res;
  if (res == 0)
    in_dco_add_que(ctx);
  else if (res == 4)
    workers_[ctx->tid_]->dco_sleep_resume(ctx);
  return res;
}

int coschedule::dco_yield(coctx *ctx,
                          const std::function<void(coctx *)> &call) {
  BOOST_ASSERT(ctx != nullptr);
  int res = ctx->try_yield_full();
  if (res < 0)
    return res;
  if (call)
    call(ctx);
  // 切换回调度，让出下cpu
  ctx->swout_();
  return 0;
}

int coschedule::dco_sleep(coctx *ctx, time_t ms,
                          const std::function<void(coctx *)> &call) {
  return dco_sleep_until(ctx, cotools::ms2until(ms), call);
}

int coschedule::dco_sleep_until(coctx *ctx, time_t ms_until,
                                const std::function<void(coctx *)> &call) {
  BOOST_ASSERT(ctx != nullptr);
  int res = ctx->try_sleep_full();
  if (res < 0)
    return res;
  if (call)
    call(ctx);
  // 设置超时信息
  ctx->set_timeout(ms_until);
  // 切换回调度
  ctx->swout_();
  return ctx->break_ ? 1 : 0;
}

int coschedule::dco_gosche(coctx *ctx) {
  // 让出协程,加入就绪队尾
  return dco_yield(ctx, [this](coctx *ctx) { dco_resume(ctx); });
}

coctx *coschedule::dco_curr_ctx() {
  // 获取当前执行的协程上下文
  return local_ctx_;
}

int coschedule::dco_thread_id() {
  // 获取当前的工作id
  return local_id_;
}

void coschedule::dco_show_worker_info() {
  int count = 0;
  for (auto worker : workers_) {
    count += worker->dco_work_count();
    printf("dco tid:%d work count:%d\n", worker->dco_work_id(), worker->dco_work_count());
  }
  printf("dco all count:%d\n", count);
}
