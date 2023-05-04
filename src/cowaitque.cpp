#include "libdco/co/cowaitque.h"

#include "libdco/util/cotools.h"

using namespace dco;

cowaitque::cowaitque(coschedule *sche)
    : ptr_sche_(sche)
#if DCO_MULT_THREAD
      ,
      mtx_ BOOST_DETAIL_SPINLOCK_INIT
#endif
{
}

cowaitque::~cowaitque() {
  BOOST_ASSERT(ctx_que_.front() == nullptr); // 这时候应该是没有东西的
}

void cowaitque::wait_set(waitnode &node) {
#if DCO_MULT_THREAD
  std::lock_guard<boost::detail::spinlock> lock_mtx(mtx_);
#endif
  ctx_que_.push_back(&node);
}

void cowaitque::wait_remove(waitnode &node) {
#if DCO_MULT_THREAD
  std::lock_guard<boost::detail::spinlock> lock_mtx(mtx_);
#endif
  ctx_que_.remove(&node);
}

#if DCO_MULT_THREAD
int cowaitque::wait(std::unique_lock<std::mutex> &lock,
                    const std::function<bool(coctx *)> &cond) {
  coctx *ctx = ptr_sche_->dco_curr_ctx();
  BOOST_ASSERT(ctx != nullptr);
  waitnode node(ctx);
  while (!cond(ctx)) {
    if (0 != ptr_sche_->dco_yield(ctx, [this, &node, &cond, &lock](coctx *ctx) {
          {
            std::lock_guard<boost::detail::spinlock> lock_mtx(mtx_);
            ctx_que_.push_back(&node);
          }
          // 等加入在解锁 防止略过唤醒
          lock.mutex()->unlock(); // unlock
        })) {
      // await fail un call callback
      // do not need relock
      BOOST_ASSERT(false); // 通知加锁失败
      return -1;
    }
    lock.mutex()->lock(); // relock
  }
  return 1; // 被打断了或者条件为真
}

int cowaitque::wait_for(std::unique_lock<std::mutex> &lock, time_t ms,
                        const std::function<bool(coctx *)> &cond) {
  time_t ms_until = cotools::ms2until(ms);
  return wait_until(lock, ms_until, cond);
}

int cowaitque::wait_until(std::unique_lock<std::mutex> &lock, time_t ms_until,
                          const std::function<bool(coctx *)> &cond) {
  coctx *ctx = ptr_sche_->dco_curr_ctx();
  BOOST_ASSERT(ctx != nullptr);
  waitnode node(ctx);
  while (!cond(ctx)) {
    int sleep_res = ptr_sche_->dco_sleep_until(
        ctx, ms_until, [this, &node, &cond, &lock](coctx *ctx) {
          {
            std::lock_guard<boost::detail::spinlock> lock_mtx(mtx_);
            ctx_que_.push_back(&node);
          }
          // 等加入在解锁 防止略过唤醒
          lock.mutex()->unlock(); // unlock
        });
    if (0 > sleep_res) {
      // await fail un call callback
      // do not need relock
      return -1; // 挂起失败
    }
    if (0 == sleep_res) {
      {
        std::lock_guard<boost::detail::spinlock> lock_mtx(mtx_);
        ctx_que_.remove(&node);
      }
      // await success need relock
      lock.mutex()->lock();
      return 0; // 超时
    }
    lock.mutex()->lock(); // relock
  }
  return 1; // 被打断了或者条件为真
}

#else
int cowaitque::wait(const std::function<bool(coctx *)> &cond) {
  coctx *ctx = ptr_sche_->dco_curr_ctx();
  BOOST_ASSERT(ctx != nullptr);
  waitnode node(ctx);
  while (!cond(ctx)) {
    if (0 != ptr_sche_->dco_yield(ctx, [this, &node, &cond](coctx *ctx) {
          ctx_que_.push_back(&node);
        })) {
      return -1;
    }
  }
  return 1; // 被打断了或者条件为真
}

int cowaitque::wait_for(time_t ms, const std::function<bool(coctx *)> &cond) {
  time_t ms_until = cotools::ms2until(ms);
  return wait_until(ms_until, cond);
}

int cowaitque::wait_until(time_t ms_until,
                          const std::function<bool(coctx *)> &cond) {
  coctx *ctx = ptr_sche_->dco_curr_ctx();
  BOOST_ASSERT(ctx != nullptr);
  waitnode node(ctx);
  while (!cond(ctx)) {
    int sleep_res = ptr_sche_->dco_sleep_until(
        ctx, ms_until,
        [this, &node, &cond](coctx *ctx) { ctx_que_.push_back(&node); });
    if (0 > sleep_res) {
      return -1; // 挂起失败
    }
    if (0 == sleep_res) {
      ctx_que_.remove(&node);
      return 0; // 超时
    }
  }
  return 1; // 被打断了或者条件为真
}

#endif

int cowaitque::notify() {
  coctx *ctx = nullptr;
  {
#if DCO_MULT_THREAD
    std::lock_guard<boost::detail::spinlock> lock_mtx(mtx_);
#endif
    waitnode *node = ctx_que_.front();
    if (node == nullptr)
      return -1;
    ctx = node->ctx_;
    ctx_que_.pop_front();
  }
  ptr_sche_->dco_resume(ctx);
  return 1;
}

int cowaitque::notify_all() {
  int times = 0;
  for (;;) {
    auto ret = notify();
    if (ret != 1)
      break;
    ++times;
  }
  return times;
}

cowaitquecas::cowaitquecas(coschedule *sche) : ptr_sche_(sche) {}

cowaitquecas::~cowaitquecas() {
  coctx *ctx = nullptr;
  BOOST_ASSERT(ctx_que_.pop(ctx) == false);
}

#if DCO_MULT_THREAD
int cowaitquecas::wait(std::unique_lock<std::mutex> &lock,
                       const std::function<bool(coctx *)> &cond) {
  coctx *ctx = ptr_sche_->dco_curr_ctx();
  BOOST_ASSERT(ctx != nullptr);
  while (!cond(ctx)) {
    if (0 != ptr_sche_->dco_yield(ctx, [this, &cond, &lock](coctx *ctx) {
          ctx_que_.push(ctx);
          lock.mutex()->unlock(); // unlock
        })) {
      // await fail un call callback
      // do not need relock
      BOOST_ASSERT(false);  // 通知加锁失败
      return -1;
    }
    lock.mutex()->lock(); // relock
  }
  return 1; // 被打断了或者条件为真
}
#else
int cowaitquecas::wait(const std::function<bool(coctx *)> &cond) {
  coctx *ctx = ptr_sche_->dco_curr_ctx();
  BOOST_ASSERT(ctx != nullptr);
  while (!cond(ctx)) {
    if (0 != ptr_sche_->dco_yield(
                 ctx, [this, &cond](coctx *ctx) { ctx_que_.push(ctx); })) {
      return -1;
    }
  }
  return 1; // 被打断了或者条件为真
}
#endif

void cowaitquecas::notify() {
  coctx *ctx = nullptr;
  ctx_que_.pop(ctx);
  ptr_sche_->dco_resume(ctx);
}

void cowaitquecas::notify_all() {
  coctx *ctx = nullptr;
  while (ctx_que_.pop(ctx))
    ptr_sche_->dco_resume(ctx);
}
