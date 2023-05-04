#include "libdco/co/comutex.h"

using namespace dco;

comutex::comutex(coschedule *sche)
    : ptr_sche_(sche), flag_(false), ctx_curr_(nullptr) {}

comutex::~comutex() {
  coctx *ctx = nullptr;
  // 这时候需要等待队列中没有任何的协程在
  BOOST_ASSERT(ctx_que_.pop(ctx) == false);
}

bool comutex::lock() {
  coctx *ctx = ptr_sche_->dco_curr_ctx();
  BOOST_ASSERT(ctx != nullptr);
  for (;;) {
    if (!flag_.exchange(true)) { // 如果返回的是false那就是可以获取锁的
      ctx_curr_ = ctx;
      break;
    }
    if (0 !=
        ptr_sche_->dco_yield(ctx, [this](coctx *ctx) { ctx_que_.push(ctx); })) {
      BOOST_ASSERT(false);  // 通知加锁失败
      return false; // 失败
    }
  }
  return true; // 获取到了锁
}

bool comutex::unlock() {
  BOOST_ASSERT(ptr_sche_->dco_curr_ctx() != nullptr &&
               ctx_curr_ == ptr_sche_->dco_curr_ctx() && flag_.load() == true);
  if (ctx_curr_ != ptr_sche_->dco_curr_ctx())
    return false;
  if (!flag_.exchange(false)) // 如果是false说明没锁定过
    return false;
  coctx *ctx = nullptr;
  ctx_que_.pop(ctx);
  ptr_sche_->dco_resume(ctx);
  return true;
}

comutex_seq::comutex_seq(coschedule *sche)
    : ptr_sche_(sche), flag_(false), ctx_curr_(nullptr)
#if DCO_MULT_THREAD
      ,
      mtx_ BOOST_DETAIL_SPINLOCK_INIT
#endif
{
}

comutex_seq::~comutex_seq() {
  // 这时候需要等待队列中没有任何的协程在
  BOOST_ASSERT(ctx_que_.front() == nullptr);
}

bool comutex_seq::lock() {
  {
#if DCO_MULT_THREAD
    std::unique_lock<boost::detail::spinlock> lock(mtx_);
#endif
    coctx *ctx = ptr_sche_->dco_curr_ctx();
    BOOST_ASSERT(ctx != nullptr);
    // 检查是否可以获取锁
    if (flag_ == false) {
      ctx_curr_ = ctx;
      flag_ = true;
      return true;
    }
    // 挂起当前协程 这里是为了防止 pop.que(try resume) set add.que(yield)
    // 无法被唤醒问题
    waitnode node(ctx);
#if DCO_MULT_THREAD
    if (0 != ptr_sche_->dco_yield(ctx, [this, &node, &lock](coctx *) {
          ctx_que_.push_back(&node);
          // yield 后解锁
          lock.mutex()->unlock(); // unlock
        })) {
      BOOST_ASSERT(false);  // 通知加锁失败
      return false;
    }
    // yield 被唤醒 需要relock
    lock.mutex()->lock(); // relock
#else
    if (0 != ptr_sche_->dco_yield(
                 ctx, [this, &node](coctx *) { ctx_que_.push_back(&node); })) {
      return false;
    }
#endif
  }
  // 被唤醒
  return true;
}

bool comutex_seq::unlock() {
  {
#if DCO_MULT_THREAD
    std::lock_guard<boost::detail::spinlock> lock(mtx_);
#endif
    BOOST_ASSERT(ptr_sche_->dco_curr_ctx() != nullptr &&
                 ctx_curr_ == ptr_sche_->dco_curr_ctx() && flag_ == true);
    if (ctx_curr_ != ptr_sche_->dco_curr_ctx())
      return false;
    if (flag_ == false)
      return false;
    // 检查是否有可以唤醒的
    waitnode *node = ctx_que_.front();
    if (node == nullptr || node->ctx_ == nullptr) {
      flag_ = false;
      return true;
    }
    // 设置为当前得到锁的协程
    ctx_curr_ = node->ctx_;
    // 出队
    ctx_que_.pop_front();
  }
  // 唤醒等待锁的
  ptr_sche_->dco_resume(ctx_curr_);
  return true;
}