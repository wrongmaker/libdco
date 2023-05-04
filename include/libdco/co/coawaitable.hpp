#pragma once

#include "libdco/co/coschedule.h"

namespace dco {
class coawait;

template <class T> class coobject {
private:
  friend class coawait;
  std::atomic<coctx *> ctx_;
  coschedule *ptr_sche_;

public:
  coobject(coschedule *sche) : ctx_(nullptr), ptr_sche_(sche) {}
  virtual ~coobject() {}

private:
  inline void check_coctx() { BOOST_ASSERT(ctx_.load() == nullptr); }
  inline void set_coctx(coctx *ctx) { ctx_.store(ctx); }
  inline coschedule *get_sche() const { return ptr_sche_; }

public:
  bool resume() {
    auto ret = false;
    coctx *ctx = ctx_.load();
    BOOST_ASSERT(ctx != nullptr);
    // 比较并置空后唤醒他
    if (ctx_.compare_exchange_strong(ctx, nullptr))
      ret = ptr_sche_->dco_resume(ctx) >= 0;
    return ret;
  }

public:
  virtual bool await_ready() = 0;
  virtual void await_suspend() = 0;
  virtual T await_resume() = 0;
  virtual T await_timeout() = 0;
  virtual uint32_t await_sleep() = 0;
};

template <> class coobject<void> {
private:
  friend class coawait;
  std::atomic<coctx *> ctx_;
  coschedule *ptr_sche_;

public:
  coobject(coschedule *sche) : ctx_(nullptr), ptr_sche_(sche) {}
  virtual ~coobject() {}

private:
  inline void check_coctx() { BOOST_ASSERT(ctx_.load() == nullptr); }
  inline void set_coctx(coctx *ctx) { ctx_.store(ctx); }
  inline coschedule *get_sche() const { return ptr_sche_; }

public:
  bool resume() {
    auto ret = false;
    coctx *ctx = ctx_.load();
    BOOST_ASSERT(ctx != nullptr);
    // 比较并置空后唤醒他
    if (ctx_.compare_exchange_strong(ctx, nullptr))
      ret = ptr_sche_->dco_resume(ctx) >= 0;
    return ret;
  }

public:
  virtual bool await_ready() = 0;
  virtual void await_suspend() = 0;
  virtual void await_resume() = 0;
  virtual void await_timeout() = 0;
  virtual uint32_t await_sleep() = 0;
};

class coawait {
public:
  template <class T> T operator=(const coobject<T> &obj) {
    coobject<T> &objc = const_cast<coobject<T> &>(obj);
    objc.check_coctx();
    if (!objc.await_ready()) {
      auto yield_call = [&objc](coctx *ctx) {
        objc.set_coctx(ctx);
        objc.await_suspend();
      };
      if (objc.await_sleep() > 0) {
        int ret = obj.get_sche()->dco_sleep(objc.get_sche()->dco_curr_ctx(),
                                            objc.await_sleep(), yield_call);
        if (ret == 0)
          return objc.await_timeout();
      } else {
        obj.get_sche()->dco_yield(objc.get_sche()->dco_curr_ctx(), yield_call);
      }
    }
    return objc.await_resume();
  }

  void operator=(const coobject<void> &obj) {
    coobject<void> &objc = const_cast<coobject<void> &>(obj);
    objc.check_coctx();
    if (!objc.await_ready()) {
      auto yield_call = [&objc](coctx *ctx) {
        objc.set_coctx(ctx);
        objc.await_suspend();
      };
      if (objc.await_sleep() > 0) {
        int ret = obj.get_sche()->dco_sleep(objc.get_sche()->dco_curr_ctx(),
                                            objc.await_sleep(), yield_call);
        if (ret == 0) {
          objc.await_timeout();
          return;
        }
      } else {
        obj.get_sche()->dco_yield(objc.get_sche()->dco_curr_ctx(), yield_call);
      }
    }
    objc.await_resume();
  }
};

} // namespace dco
