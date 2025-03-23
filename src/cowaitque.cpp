#include "libdco/co/cowaitque.h"

#include "libdco/co/all.h"
#include "libdco/co/cofutex.h"
#include "libdco/util/cotools.h"
#include <cstddef>

using namespace dco;

cowaitque_mt::cowaitque_mt(coschedule *sche) : ptr_sche_(sche), stop_(false) {}

cowaitque_mt::~cowaitque_mt() {
  BOOST_ASSERT(!ctx_que_.pop(nullptr)); // 这时候应该是没有东西的
}

int cowaitque_mt::wait_set(waitnode &node) {
  if (stop_.load())
    return -2;
  ctx_que_.push(&node);
  return 1;
}

void cowaitque_mt::wait_remove(waitnode &node) { ctx_que_.remove(&node); }

bool cowaitque_mt::empty() { return ctx_que_.empty(); }

int cowaitque_mt::wait(void *payload,
                       const cowaitque_mt::check_func &check_call,
                       const cowaitque_mt::cond_func &cond_call) {
  coctx *ctx = ptr_sche_->dco_curr_ctx();
  BOOST_ASSERT(ctx != nullptr);
  waitnode node(ctx, payload);
  while (!node.unpack_ && !cond_call(payload)) {
    int res = dco_futex::dco_wait(ptr_sche_, ctx_que_, node, stop_, check_call);
    if (-2 == res)
      return res;
  }
  return 1; // 被打断了或者条件为真
}

int cowaitque_mt::wait_for(time_t ms, void *payload,
                           const check_func &check_call,
                           const cond_func &cond_call) {
  time_t ms_until = cotools::ms2until(ms);
  return wait_until(ms_until, payload, check_call, cond_call);
}

int cowaitque_mt::wait_until(time_t ms_until, void *payload,
                             const check_func &check_call,
                             const cond_func &cond_call) {
  coctx *ctx = ptr_sche_->dco_curr_ctx();
  BOOST_ASSERT(ctx != nullptr);
  waitnode node(ctx, payload);
  while (!node.unpack_ && !cond_call(payload)) {
    int res = dco_futex::dco_wait_until(ptr_sche_, ms_until, ctx_que_, node,
                                        stop_, check_call);
    if (-2 == res)
      return res;
    if (0 == res && !node.unpack_)
      return res; // 如果unpack了说明数据已经传递过了，即使超时也算成功
  }
  return 1; // 被打断了或者条件为真
}

int cowaitque_mt::call(const unpack_func &node_call) {
  if (node_call) {
    return dco_futex::dco_call<cowaitque_mt::waitnode>(
        ptr_sche_, ctx_que_, [&node_call](waitnode *node) -> bool {
          node->unpack_ = true;
          node_call(node->payload_);
          return true;
        });
  }
  return dco_futex::dco_notify(ptr_sche_, ctx_que_);
}

void cowaitque_mt::close() {
  stop_.store(true);
  // 唤醒所有
  dco_futex::dco_notify_all(ptr_sche_, ctx_que_);
}
