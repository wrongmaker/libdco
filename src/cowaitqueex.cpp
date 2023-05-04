#include "libdco/co/cowaitqueex.h"

#include "libdco/util/cotools.h"

using namespace dco;

cowaitqueex::cowaitqueex(coschedule *sche)
    : ptr_sche_(sche),
#if DCO_MULT_THREAD
      mtx_ BOOST_DETAIL_SPINLOCK_INIT,
#endif
      stop_(false) {
}

cowaitqueex::~cowaitqueex() {
  BOOST_ASSERT(ctx_que_.front() == nullptr); // 这时候应该是没有东西的
}

void cowaitqueex::wait_set(waitnode &node) {
#if DCO_MULT_THREAD
  std::lock_guard<boost::detail::spinlock> lock_mtx(mtx_);
#endif
  ctx_que_.push_back(&node);
}

void cowaitqueex::wait_remove(waitnode &node) {
#if DCO_MULT_THREAD
  std::lock_guard<boost::detail::spinlock> lock_mtx(mtx_);
#endif
  ctx_que_.remove(&node);
}

#if DCO_MULT_THREAD
int cowaitqueex::wait(std::unique_lock<std::mutex> &lock, const void *payload) {
  coctx *ctx = ptr_sche_->dco_curr_ctx();
  BOOST_ASSERT(ctx != nullptr);
  waitnode node(ctx, payload);
  if (0 != ptr_sche_->dco_yield(ctx, [this, &node, &lock](coctx *) {
        {
          std::lock_guard<boost::detail::spinlock> lock_mtx(mtx_);
          ctx_que_.push_back(&node);
        }
        // 等加入在解锁 防止略过唤醒
        lock.mutex()->unlock(); // unlock
      })) {
    // await fail un call callback
    // do not need relock
    BOOST_ASSERT(false);  // 通知加锁失败
    return -1;
  }
  lock.mutex()->lock(); // relock
  return 1;             // 被打断了或者条件为真
}

int cowaitqueex::wait_for(std::unique_lock<std::mutex> &lock, time_t ms,
                          const void *payload) {
  time_t ms_until = cotools::ms2until(ms);
  return wait_until(lock, ms_until, payload);
}

int cowaitqueex::wait_until(std::unique_lock<std::mutex> &lock, time_t ms_until,
                            const void *payload) {
  coctx *ctx = ptr_sche_->dco_curr_ctx();
  BOOST_ASSERT(ctx != nullptr);
  waitnode node(ctx, payload);
  int sleep_res = ptr_sche_->dco_sleep_until(
      ctx, ms_until, [this, &node, &lock](coctx *ctx) {
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
  return 1;             // 被打断了或者条件为真
}

#else
int cowaitqueex::wait(const void *payload) {
  coctx *ctx = ptr_sche_->dco_curr_ctx();
  BOOST_ASSERT(ctx != nullptr);
  waitnode node(ctx, payload);
  if (0 != ptr_sche_->dco_yield(
               ctx, [this, &node](coctx *ctx) { ctx_que_.push_back(&node); })) {
    return -1;
  }
  return 1; // 被打断了或者条件为真
}

int cowaitqueex::wait_for(time_t ms, const void *payload) {
  time_t ms_until = cotools::ms2until(ms);
  return wait_until(ms_until, payload);
}

int cowaitqueex::wait_until(time_t ms_until, const void *payload) {
  coctx *ctx = ptr_sche_->dco_curr_ctx();
  BOOST_ASSERT(ctx != nullptr);
  waitnode node(ctx, payload);
  int sleep_res = ptr_sche_->dco_sleep_until(
      ctx, ms_until, [this, &node](coctx *ctx) { ctx_que_.push_back(&node); });
  if (0 > sleep_res) {
    return -1; // 挂起失败
  }
  if (0 == sleep_res) {
    ctx_que_.remove(&node);
    return 0; // 超时
  }
  return 1; // 被打断了或者条件为真
}

#endif

int cowaitqueex::notify(const std::function<void(const void *)> &call) {
  coctx *ctx = nullptr;
  const void *payload = nullptr;
  {
#if DCO_MULT_THREAD
    std::lock_guard<boost::detail::spinlock> lock_mtx(mtx_);
#endif
    waitnode *node = ctx_que_.front();
    if (node == nullptr)
      return -1;
    ctx = node->ctx_;
    payload = node->payload_;
    if (node->call_ != nullptr)
      node->call_();
    ctx_que_.pop_front();
  }
  call(payload);
  ptr_sche_->dco_resume(ctx);
  return 1;
}