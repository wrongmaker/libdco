#pragma once
#include "libdco/co/coschedule.h"
#include "libdco/util/cocasque.hpp"
#include "libdco/util/conolist.hpp"
#include <atomic>

namespace dco {
// 在有wait,post阻塞协程阻塞的情况下不能释放cowaitque
// 若非全局推荐用智能指针管理
// 线程安全
// 带锁等待队列,不允许超时,否则可能会超时调度时，notify还在
class cowaitque_mt {
public:
  typedef std::function<bool()> check_func;
  typedef std::function<bool(void *)> cond_func;
  typedef std::function<void(const void *)> unpack_func;

public:
  struct waitnode {
    waitnode *prev_;
    waitnode *next_;
    coctx *ctx_;
    const void *payload_;
    bool unpack_;
    waitnode(coctx *ctx, const void *payload)
        : prev_(nullptr), next_(nullptr), ctx_(ctx), payload_(payload),
          unpack_(false) {}
    waitnode() = delete;
    waitnode(const waitnode &) = delete;
  };

private:
  coschedule *ptr_sche_;
  std::atomic<bool> stop_;
  conolist_mt<waitnode> ctx_que_; // 等待队列

public:
  cowaitque_mt(coschedule *);
  cowaitque_mt() = delete;
  cowaitque_mt(const cowaitque_mt &) = delete;
  ~cowaitque_mt();

public:
  // 1:成功 -2: 关闭
  int wait_set(waitnode &node);     // 添加节点
  void wait_remove(waitnode &node); // 移除节点

public:
  bool empty();
  // check_call 不操作，只进行检查 @return true 则会被唤醒并执行 cond_call
  // cond_call 检查并执行操作 @return true 则表示执行成功
  // -1:失败 1:成功 -2:关闭
  int wait(void *payload, const check_func &check_call,
           const cond_func &cond_call);
  // -1:失败 0:超时 1:成功 -2:关闭
  int wait_for(time_t ms, void *payload, const check_func &check_call,
               const cond_func &cond_call);
  int wait_until(time_t ms_until, void *payload, const check_func &check_call,
                 const cond_func &cond_call);
  // 1:成功 -1:失败 -2:关闭
  int call(const unpack_func &node_call = nullptr);
  void close();
};

} // namespace dco
