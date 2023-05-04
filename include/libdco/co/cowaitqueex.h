#pragma once
#include "libdco/co/coschedule.h"
#include "libdco/util/cocasque.hpp"
#include "libdco/util/conolist.hpp"

namespace dco {
// 在有wait,post阻塞协程阻塞的情况下不能释放cowaitque
// 若非全局推荐用智能指针管理
// 线程安全
// 带锁等待队列,带超时等待
class cowaitqueex {
public:
  struct waitnode {
    waitnode *prev_;
    waitnode *next_;
    coctx *ctx_;
    const void *payload_;
    std::function<void()> call_;
    waitnode(coctx *ctx, const void *payload,
             std::function<void()> call = nullptr)
        : prev_(nullptr), next_(nullptr), ctx_(ctx), payload_(payload),
          call_(call) {}
    waitnode() = delete;
    waitnode(const waitnode &) = delete;
  };

private:
  coschedule *ptr_sche_;
#if DCO_MULT_THREAD
  mutable boost::detail::spinlock mtx_;
#endif
  volatile bool stop_;
  conolist<waitnode> ctx_que_; // 等待队列

public:
  cowaitqueex(coschedule *);
  cowaitqueex() = delete;
  cowaitqueex(const cowaitqueex &) = delete;
  ~cowaitqueex();

public:
  void wait_set(waitnode &node);    // 添加节点
  void wait_remove(waitnode &node); // 移除节点

public:
#if DCO_MULT_THREAD
  int wait(std::unique_lock<std::mutex> &lock,
           const void *payload); // -1:失败 1:成功
  // 结果为false会进行等待
  int wait_for(std::unique_lock<std::mutex> &lock, time_t ms,
               const void *payload); // -1:失败 0:超时 1:成功
  int wait_until(std::unique_lock<std::mutex> &lock, time_t ms_until,
                 const void *payload); // -1:失败 0:超时 1:成功
#else
  int wait(const void *payload); // -1:失败 1:成功
  // 结果为false会进行等待
  int wait_for(time_t ms, const void *payload); // -1:失败 0:超时 1:成功
  int wait_until(time_t ms_until,
                 const void *payload); // -1:失败 0:超时 1:成功
#endif
  int notify(const std::function<void(const void *)> &call); // 1:成功 -1:失败
};

} // namespace dco
