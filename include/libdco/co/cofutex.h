#pragma once

#include "boost/smart_ptr/shared_ptr.hpp"
#include "libdco/co/coctx.h"
#include "libdco/co/coschedule.h"
#include "libdco/util/conolist.hpp"
#include <functional>

namespace dco {

class dco_futex {
public:
  template <class T>
  static int dco_wait(coschedule *ptr_sche, conolist<T> &list, T &node,
                      const std::function<bool()> &check) {
    dco::coctx *ctx = node.ctx_;
    BOOST_ASSERT(ctx != nullptr);
    if (0 !=
        ptr_sche->dco_yield(ctx, [ptr_sche, &list, &node, check](coctx *ctx) {
          if (check()) {
            ptr_sche->dco_resume(ctx);
            return;
          }
          list.push(&node);
          if (check())
            dco_notify(ptr_sche, list);
        })) {
      BOOST_ASSERT(false); // 通知加锁失败
      return -1;
    }
    return 1; // 被打断了或者条件为真
  }

  template <class T>
  static int dco_wait_until(coschedule *ptr_sche, time_t ms_until,
                            conolist<T> &list, T &node,
                            const std::function<bool()> &check) {
    dco::coctx *ctx = node.ctx_;
    BOOST_ASSERT(ctx != nullptr);
    int sleep_res = ptr_sche->dco_sleep_until(
        ctx, ms_until, [ptr_sche, &list, &node, check](coctx *ctx) {
          if (check()) {
            ptr_sche->dco_resume(ctx);
            return;
          }
          list.push(&node);
          if (check())
            dco_notify(ptr_sche, list);
        });
    if (0 > sleep_res) {
      BOOST_ASSERT(false); // 通知加锁失败
      return -1;           // 挂起失败
    } else if (0 == sleep_res) {
      list.remove(&node);
      return 0; // 超时
    }
    return 1;
  }

  template <class T>
  static int dco_wait(coschedule *ptr_sche, conolist<T> &list, T &node,
                      std::atomic<bool> &stop,
                      const std::function<bool()> &check) {
    if (stop.load())
      return -2;
    dco::coctx *ctx = node.ctx_;
    BOOST_ASSERT(ctx != nullptr);
    if (0 != ptr_sche->dco_yield(
                 ctx, [ptr_sche, &list, &node, &stop, check](coctx *ctx) {
                   if (check() || stop.load()) {
                     ptr_sche->dco_resume(ctx);
                     return;
                   }
                   list.push(&node);
                   if (stop.load()) {
                     dco_notify_all(ptr_sche, list);
                     return;
                   }
                   if (check())
                     dco_notify(ptr_sche, list);
                 })) {
      BOOST_ASSERT(false); // 通知加锁失败
      return -1;
    } else if (stop.load()) {
      // 停止
      return -2;
    }
    return 1; // 被打断了或者条件为真
  }

  template <class T>
  static int dco_wait_until(coschedule *ptr_sche, time_t ms_until,
                            conolist<T> &list, T &node, std::atomic<bool> &stop,
                            const std::function<bool()> &check) {
    if (stop.load())
      return -2;
    dco::coctx *ctx = node.ctx_;
    BOOST_ASSERT(ctx != nullptr);
    int sleep_res = ptr_sche->dco_sleep_until(
        ctx, ms_until, [ptr_sche, &list, &node, &stop, check](coctx *ctx) {
          if (check() || stop.load()) {
            ptr_sche->dco_resume(ctx);
            return;
          }
          list.push(&node);
          if (stop.load()) {
            dco_notify_all(ptr_sche, list);
            return;
          }
          if (check())
            dco_notify(ptr_sche, list);
        });
    if (0 > sleep_res) {
      BOOST_ASSERT(false); // 通知加锁失败
      return -1;           // 挂起失败
    } else if (0 == sleep_res) {
      list.remove(&node);
      return 0; // 超时
    } else if (stop.load()) {
      // 停止
      return -2;
    }
    return 1;
  }

  template <class T>
  static int dco_wait(coschedule *ptr_sche, conolist<T> &list, T &node,
                      const std::atomic<int> &sem) {
    dco::coctx *ctx = node.ctx_;
    BOOST_ASSERT(ctx != nullptr);
    if (0 !=
        ptr_sche->dco_yield(ctx, [ptr_sche, &list, &node, &sem](coctx *ctx) {
          if (sem.load() > 0) {
            ptr_sche->dco_resume(ctx);
            return;
          }
          list.push(&node);
          if (sem.load() > 0)
            dco_notify(ptr_sche, list);
        })) {
      BOOST_ASSERT(false); // 通知加锁失败
      return -1;
    }
    return 1; // 被打断了或者条件为真
  }

  template <class T>
  static int dco_wait_until(coschedule *ptr_sche, time_t ms_until,
                            conolist<T> &list, T &node,
                            const std::atomic<int> &sem) {
    dco::coctx *ctx = node.ctx_;
    BOOST_ASSERT(ctx != nullptr);
    int sleep_res = ptr_sche->dco_sleep_until(
        ctx, ms_until, [ptr_sche, &list, &node, &sem](coctx *ctx) {
          if (sem.load() > 0) {
            ptr_sche->dco_resume(ctx);
            return;
          }
          list.push(&node);
          if (sem.load() > 0)
            dco_notify(ptr_sche, list);
        });
    if (0 > sleep_res) {
      BOOST_ASSERT(false); // 通知加锁失败
      return -1;           // 挂起失败
    } else if (0 == sleep_res) {
      list.remove(&node);
      return 0; // 超时
    }
    return 1;
  }

  // 这种带超时的结构有可能节点还在队列却超时的情况。
  template <class T>
  static bool dco_notify(coschedule *ptr_sche, conolist<T> &list) {
    return list.pop([ptr_sche](T *node) {
      coctx *ctx = node->ctx_;
      // 可能因为超时等各种原因唤醒失败，只能用来判断是否唤醒成功，后续不能在使用ctx这个节点操作，可能已经调度了
      return ptr_sche->dco_resume(ctx) >= 0;
    });
  }

  template <class T>
  static void dco_notify_all(coschedule *ptr_sche, conolist<T> &list) {
    for (;;) {
      if (!list.pop([ptr_sche](T *node) {
            ptr_sche->dco_resume(node->ctx_);
            return true;
          })) {
        break;
      }
    }
  }

  template <class T>
  static bool dco_call(coschedule *ptr_sche, conolist<T> &list,
                       const std::function<bool(T *)> &node_call) {
    // 这里的pop要和remove保持串行。
    return list.pop([ptr_sche, node_call](T *node) {
      coctx *ctx = node->ctx_;
      BOOST_ASSERT(ctx != nullptr);
      BOOST_ASSERT(node_call != nullptr);
      // BOOST_ASSERT(ctx->in_yield());
      // 这里说明这个节点还在队列中，这里因为是在队列锁中，所以是线性执行的
      // 这里可以认为这个node阶段还是存在的
      // 这里需要先执行一下node_call
      bool call_ret = node_call(node);
      // 再唤醒这个协程
      bool resume_ret = ptr_sche->dco_resume(ctx) >= 0;
      // 如果唤醒失败，但是处理成功也算成功
      return resume_ret || call_ret;
    });
  }

  static int dco_wait(coschedule *ptr_sche, cocasque<coctx *> &que,
                      const std::function<bool()> &check);
  static int dco_wait(coschedule *ptr_sche, cocasque<coctx *> &que,
                      std::atomic<bool> &flag);
  static int dco_wait(coschedule *ptr_sche, cocasque<coctx *> &que,
                      std::atomic<int> &sem);
  static int dco_notify(coschedule *ptr_sche, cocasque<coctx *> &que);
  static void dco_notify_all(coschedule *ptr_sche, cocasque<coctx *> &que);
};
} // namespace dco
