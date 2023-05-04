#pragma once
#include "libdco/co/coschedule.h"

namespace dco {

// 生成器

template <class Main_, class Func_> class cogenerator_yield;

template <class Main_, class Func_> class cogenerator {
private:
  coschedule *ptr_sche_;
  coctx *main_ctx_;
  coctx *func_ctx_;
  Main_ main_arg_;
  Func_ func_arg_;
  volatile bool end_;
  cogenerator_yield<Main_, Func_> *other_;

private:
  friend class cogenerator_yield<Main_, Func_>;

public:
  cogenerator(const cogenerator &) = delete;
  cogenerator(coschedule *sche)
      : ptr_sche_(sche), main_ctx_(nullptr), func_ctx_(nullptr), main_arg_(),
        func_arg_(), end_(true), other_(nullptr) {}
  ~cogenerator() { BOOST_ASSERT(func_ctx_ == nullptr); }

private:
  inline Main_ dco_yield_func(Func_ in) {
    func_arg_ = in;
    return dco_yield_func();
  }

  inline Main_ dco_yield_func() {
    ptr_sche_->dco_yield(func_ctx_,
                         [this](coctx *) { ptr_sche_->dco_resume(main_ctx_); });
    return main_arg_;
  }

  inline void dco_func_exit() {
    func_ctx_ = nullptr;
    end_ = true;
    ptr_sche_->dco_resume(main_ctx_);
  }

public:
  void
  create(const std::function<void(cogenerator_yield<Main_, Func_> &)> &func) {
    BOOST_ASSERT(func_ctx_ == nullptr);
    main_ctx_ = ptr_sche_->dco_curr_ctx();
    BOOST_ASSERT(main_ctx_ != nullptr);
    func_ctx_ = ptr_sche_->dco_create([this, func](coctx *) {
      cogenerator_yield<Main_, Func_> gen(this);
      other_ = &gen;
      func(gen);
      gen.other_->dco_func_exit();
      gen.other_ = nullptr;
    });
    end_ = false;
    ptr_sche_->dco_yield(
        main_ctx_, [this](coctx *ctx) { ptr_sche_->dco_resume(func_ctx_); });
  }

  Func_ yield() {
    ptr_sche_->dco_yield(main_ctx_,
                         [this](coctx *) { ptr_sche_->dco_resume(func_ctx_); });
    return func_arg_;
  }

  Func_ send(Main_ in) {
    main_arg_ = in;
    return yield();
  }

  Func_ operator()() { return yield(); }

  Func_ operator()(Main_ in) { return send(in); }

public:
  class iterator {
  private:
    cogenerator<Main_, Func_> &gen_;

  public:
    iterator(cogenerator<Main_, Func_> &gen) : gen_(gen) {}
    iterator &operator=(const iterator &iter) { gen_ = iter.gen_; }
    bool operator!=(const iterator &iter) { return !gen_.end_; }
    Func_ operator*() { return gen_.func_arg_; }
    iterator &operator++() {
      gen_.yield();
      return *this;
    }
  };

  iterator begin() { return iterator(*this); }

  iterator end() { return iterator(*this); }
};

template <class Main_, class Func_> class cogenerator_yield {
private:
  cogenerator<Main_, Func_> *other_;

private:
  friend class cogenerator<Main_, Func_>;

public:
  cogenerator_yield(const cogenerator_yield<Main_, Func_> &) = delete;
  cogenerator_yield(cogenerator<Main_, Func_> *other) : other_(other) {}

public:
  Main_ yield(Func_ in) { return other_->dco_yield_func(in); }

  Main_ yield() { return other_->dco_yield_func(); }

  Main_ operator()() { return yield(); }

  Main_ operator()(Func_ in) { return yield(in); }
};

} // namespace dco
