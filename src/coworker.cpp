#include "libdco/co/coworker.h"
#include "boost/smart_ptr/shared_ptr.hpp"
#include "libdco/co/coschedule.h"
#include "libdco/util/cotools.h"

using namespace dco;

coworker::coworker(coschedule *sche, int tid)
    : sche_(sche), tid_(tid), sem_(1), run_(false), wait_(true) {}

coctx *coworker::dco_get_next() {
  // 准备执行
  for (;;) {
    time_t timewait = dco_time_proc();
    // 获取一个协程
    coctx *ctx = nullptr;
    if (sche_->ctx_que_.pop(ctx)) {
      work_count_ += 1;
      return ctx;
    }
    // 标记当前工作线程在等待
    sche_->in_dco_wait_worker(tid_);
    // 尝试进行等待
    if (timewait == -1)
      sem_.try_wait();
    else
      sem_.try_wait_for(timewait);
  }
  // 返回结果
  return nullptr;
}

void coworker::dco_sleep_resume(coctx *ctx) {
  time_remove_.push(ctx);
  sem_.try_notify();
}

void coworker::dco_sleep_remove(coctx *ctx) {
  BOOST_ASSERT(ctx != nullptr);
  // 标记移除
  if (ctx->try_sleep_remove_full() != 0)
    return;
  // 表示打断并唤醒
  ctx->break_ = true;
  sche_->in_dco_add_que(ctx);
}

void coworker::dco_sleep_add(coctx *ctx) {
  BOOST_ASSERT(ctx != nullptr);
  // 标记添加
  int res = ctx->try_sleep_add_full();
  if (res < 0)
    return;
  // 已经被唤醒了
  if (res == 0) {
    sche_->in_dco_add_que(ctx);
    return;
  }
  // 检查是否从超时了
  time_t now = cotools::msnow();
  if (ctx->timeout_ <= now)
    dco_sleep_timeout(ctx);
  else
    time_wait_.push(ctx);
}

void coworker::dco_sleep_timeout(coctx *ctx) {
  BOOST_ASSERT(ctx != nullptr);
  // 标记超时
  if (ctx->try_sleep_timeout_full() != 0)
    return;
  // 表示超时并唤醒
  ctx->break_ = false;
  sche_->in_dco_add_que(ctx); // 直接唤醒
}

time_t coworker::dco_time_proc() {
  time_t now = cotools::msnow();
  // 检查是否有移除
  for (;;) {
    coctx *ctx = nullptr;
    if (!time_remove_.pop(ctx))
      break;
    time_wait_.remove(ctx); // 防止重复索引
    dco_sleep_remove(ctx);  // 移除定时器
  }
  // 检查是否有超时
  for (;;) {
    coctx *ctx = time_wait_.top();
    if (ctx == nullptr)
      return -1;
    // 将超时的协程移除并处理
    if (ctx->timeout_ <= now) {
      time_wait_.remove(ctx);
      dco_sleep_timeout(ctx);
      continue;
    }
    // 等待时间
    return ctx->timeout_ - now;
  }
  return -1;
}

int coworker::dco_work_id() {
  // 获取任务id
  return tid_;
}

int coworker::dco_work_count() {
  // 执行的任务个数
  return work_count_;
}
