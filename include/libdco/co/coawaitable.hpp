#pragma once

#include "libdco/co/coschedule.h"
#include <optional>

namespace dco {
class coawait;
template <typename T> class coobject;
template <typename T> class coobject_handle;

template <typename T> class coobject_context {
private:
  coschedule *ptr_sche_;
  std::atomic<coctx *> ctx_;
  T t_;

  friend class coobject_handle<T>;
  friend class coobject<T>;
  friend class coawait;

private:
  void ctx_set(coctx *ctx) { ctx_.store(ctx); }
  void ctx_clear() { ctx_.store(nullptr); }
  void resume(T &&t) {
    coctx *ctx = ctx_.load();
    if (ctx == nullptr || !ctx_.compare_exchange_strong(ctx, nullptr))
      return;
    t_ = std::move(t);
    ptr_sche_->dco_resume(ctx);
  }

public:
  coobject_context(const coobject_context &) = delete;
  coobject_context() = delete;
  coobject_context(coschedule *ptr_sche) : ptr_sche_(ptr_sche) {}
  ~coobject_context() {}
};

template <> class coobject_context<void> {
private:
  coschedule *ptr_sche_;
  std::atomic<coctx *> ctx_;

  friend class coobject_handle<void>;
  friend class coobject<void>;
  friend class coawait;

private:
  void ctx_set(coctx *ctx) { ctx_.store(ctx); }
  void ctx_clear() { ctx_.store(nullptr); }
  void resume(void) {
    coctx *ctx = ctx_.load();
    if (ctx == nullptr || !ctx_.compare_exchange_strong(ctx, nullptr))
      return;
    ptr_sche_->dco_resume(ctx);
  }

public:
  coobject_context(const coobject_context &) = delete;
  coobject_context() = delete;
  coobject_context(coschedule *ptr_sche) : ptr_sche_(ptr_sche) {}
  ~coobject_context() {}
};

template <typename T> class coobject_handle {
private:
  std::shared_ptr<coobject_context<T>> context_;

public:
  void operator()(T &&t) {
    if (context_)
      context_->resume(std::move(t));
  }

  operator bool() const { return context_ != nullptr; }

public:
  coobject_handle() = delete;
  coobject_handle(std::shared_ptr<coobject_context<T>> context)
      : context_(context) {}
  coobject_handle(const coobject_handle<T> &obj) : context_(obj.context_) {}
  ~coobject_handle() {}
};

template <> class coobject_handle<void> {
private:
  std::shared_ptr<coobject_context<void>> context_;

public:
  void operator()(void) {
    if (context_)
      context_->resume();
  }

  operator bool() const { return context_ != nullptr; }

public:
  coobject_handle() = delete;
  coobject_handle(std::shared_ptr<coobject_context<void>> context)
      : context_(context) {}
  coobject_handle(const coobject_handle<void> &obj) : context_(obj.context_) {}
  ~coobject_handle() {}
};

template <typename T> class coobject {
private:
  friend class coawait;

  std::shared_ptr<coobject_context<T>> context_;

public:
  virtual void Suspend(coobject_handle<T> handle) = 0;
  virtual void TimeOut() = 0;
  virtual uint32_t TimeWait() const = 0;

public:
  coobject() = delete;
  coobject(coschedule *sche) {
    context_ = std::make_shared<coobject_context<T>>(sche);
  }
  coobject(const coobject<T> &obj) : context_(obj.context_) {}
  virtual ~coobject() {}
};

struct coawait {
  template <typename T>
  std::optional<T> operator=(const coobject<T> &await_obj) {
    coobject<T> &await = const_cast<coobject<T> &>(await_obj);
    coschedule *sche = await.context_->ptr_sche_;
    coctx *curr_ctx = sche->dco_curr_ctx();
    auto yield_call = [&await](coctx *ctx) {
      await.context_->ctx_set(ctx);
      await.Suspend({await.context_});
    };
    if (await.TimeWait() > 0) {
      int ret = sche->dco_sleep(curr_ctx, await.TimeWait(), yield_call);
      if (ret == 0) {
        await.context_->ctx_clear();
        await.TimeOut();
        return {};
      }
    } else {
      sche->dco_yield(curr_ctx, yield_call);
    }
    return {await.context_->t_};
  }

  void operator=(const coobject<void> &await_obj) {
    coobject<void> &await = const_cast<coobject<void> &>(await_obj);
    coschedule *sche = await.context_->ptr_sche_;
    coctx *curr_ctx = sche->dco_curr_ctx();
    auto yield_call = [&await](coctx *ctx) {
      await.context_->ctx_set(ctx);
      await.Suspend({await.context_});
    };
    if (await.TimeWait() > 0) {
      int ret = sche->dco_sleep(curr_ctx, await.TimeWait(), yield_call);
      if (ret == 0) {
        await.context_->ctx_clear();
        await.TimeOut();
        return;
      }
    } else {
      sche->dco_yield(curr_ctx, yield_call);
    }
    return;
  }
};

} // namespace dco
