#pragma once
#include "libdco/co/cowaitqueex.h"
#include "libdco/util/cotools.h"

namespace dco {

class coselect;

class cochannel_base {
public:
  virtual void wait_remove(cowaitqueex::waitnode &node) = 0;
  virtual void post_remove(cowaitqueex::waitnode &node) = 0;
  virtual int wait_check(cowaitqueex::waitnode &node, bool hook = true) = 0;
  virtual int post_check(cowaitqueex::waitnode &node, bool hook = true) = 0;
};

template <class Val_> class cochannel : public cochannel_base {
private:
  int size_;
  int max_;
  cowaitqueex wait_;
  cowaitqueex post_;
  bool stop_;
  coschedule *ptr_sche_;
  cocasque<Val_> buf_que_;
#if DCO_MULT_THREAD
  std::mutex mtx_;
#endif

private:
  friend class coselect;

public:
  cochannel() = delete;
  cochannel(const cochannel &) = delete;
  cochannel(coschedule *sche, int max = 0)
      : size_(0), max_(max), wait_(sche), post_(sche), stop_(false),
        ptr_sche_(sche) {}
  ~cochannel() {}

public:
  struct ec_cochannel {
    int ec_;
    Val_ &inf_;
    ec_cochannel(Val_ &inf) : ec_(0), inf_(inf) {}
  };

private:
  inline bool copy_from_buff(Val_ &val) {
    if (size_ <= 0)
      return false;
    size_ -= 1;
    buf_que_.pop(val);
    return true;
  }

  inline bool copy_to_buff(const Val_ &val) {
    if (size_ >= max_)
      return false;
    size_ += 1;
    buf_que_.push(val);
    return true;
  }

  inline int code_return(int code) {
    if (stop_)
      return -2;
    BOOST_ASSERT(code >= 0);
    return code;
  }

  inline int wait_check_real(Val_ &val) {
    // 先尝试从buff中拷贝
    if (copy_from_buff(val)) {
      // 唤醒一个post 将他数据拷贝到buff 这里被上面拿了肯定可以拷贝成功
      post_.notify(
          [this](const void *payload) { copy_to_buff(*(Val_ *)payload); });
      // copy from buff success
      return 1;
    }
    // 再尝试从post中拷贝
    auto call = [&val](const void *payload) { val = *(Val_ *)payload; };
    if (1 == post_.notify(call)) {
      // copy from post success
      return 1;
    }
    return 0;
  }

  inline int post_check_real(const Val_ &val) {
    // 先尝试拷贝到wait
    auto call = [&val](const void *payload) { *(Val_ *)payload = val; };
    if (1 == wait_.notify(call)) {
      // copy to wait success
      return 1;
    }
    // 再尝试拷贝到buff
    if (copy_to_buff(val)) {
      // copy to buff success;
      return 1;
    }
    return 0;
  }

public:
  // -2:关闭,0:未完成,1:成功
  int wait_check(cowaitqueex::waitnode &node, bool hook = true) {
    int ret = 0;
#if DCO_MULT_THREAD
    std::unique_lock<std::mutex> lock(mtx_);
#endif
    do {
      if (1 == wait_check_real(*(Val_ *)node.payload_)) {
        ret = 1;
        break;
      }
      if (stop_) {
        ret = -2;
        break;
      }
    } while (0);
    if (hook) {
      if (0 != ret) {
        ptr_sche_->dco_resume(node.ctx_);
      } else {
        wait_.wait_set(node);
      }
    }
    return ret;
  }

  // -2:关闭,0:未完成,1:成功
  int post_check(cowaitqueex::waitnode &node, bool hook = true) {
    int ret = 0;
#if DCO_MULT_THREAD
    std::unique_lock<std::mutex> lock(mtx_);
#endif
    do {
      if (1 == post_check_real(*(Val_ *)node.payload_)) {
        ret = 1;
        break;
      }
      if (stop_) {
        ret = -2;
        break;
      }
    } while (0);
    if (hook) {
      if (0 != ret) {
        ptr_sche_->dco_resume(node.ctx_);
      } else {
        post_.wait_set(node);
      }
    }
    return ret;
  }

  void wait_remove(cowaitqueex::waitnode &node) { wait_.wait_remove(node); }

  void post_remove(cowaitqueex::waitnode &node) { post_.wait_remove(node); }

  // -2:关闭,-1:失败,1:成功
  int wait(Val_ &val) {
#if DCO_MULT_THREAD
    std::unique_lock<std::mutex> lock(mtx_);
#endif
    if (1 == wait_check_real(val))
      return 1;
    if (stop_)
      return -2;
#if DCO_MULT_THREAD
    return code_return(wait_.wait(lock, std::addressof(val)));
#else
    return code_return(wait_.wait(std::addressof(val)));
#endif
  }

  // -2:关闭,-1:失败,1:成功,0:超时
  int wait_for(time_t ms, Val_ &val) {
    time_t ms_until = cotools::ms2until(ms);
    return wait_until(ms_until, val);
  }

  // -2:关闭,-1:失败,1:成功,0:超时
  int wait_until(time_t ms_until, Val_ &val) {
#if DCO_MULT_THREAD
    std::unique_lock<std::mutex> lock(mtx_);
#endif
    if (1 == wait_check_real(val))
      return 1;
    if (stop_)
      return -2;
#if DCO_MULT_THREAD
    return code_return(wait_.wait_until(lock, ms_until, std::addressof(val)));
#else
    return code_return(wait_.wait_until(ms_until, std::addressof(val)));
#endif
  }

  // -2:关闭,-1:失败,1:成功
  int post(const Val_ &val) {
#if DCO_MULT_THREAD
    std::unique_lock<std::mutex> lock(mtx_);
#endif
    if (1 == post_check_real(val))
      return 1;
    if (stop_)
      return -2;
#if DCO_MULT_THREAD
    return code_return(post_.wait(lock, std::addressof(val)));
#else
    return code_return(post_.wait(std::addressof(val)));
#endif
  }

  // -2:关闭,-1:失败,1:成功,0:超时
  int post_for(time_t ms, const Val_ &val) {
    time_t ms_until = cotools::ms2until(ms);
    return post_until(ms_until, val);
  }

  // -2:关闭,-1:失败,1:成功,0:超时
  int post_until(time_t ms_until, const Val_ &val) {
#if DCO_MULT_THREAD
    std::unique_lock<std::mutex> lock(mtx_);
#endif
    if (1 == post_check_real(val))
      return 1;
    if (stop_)
      return -2;
#if DCO_MULT_THREAD
    return code_return(post_.wait_until(lock, ms_until, std::addressof(val)));
#else
    return code_return(post_.wait_until(ms_until, std::addressof(val)));
#endif
  }

  cochannel const &operator<<(const Val_ &val) {
    post(val);
    return *this;
  }

  cochannel const &operator>>(Val_ &val) {
    wait(val);
    return *this;
  }

  cochannel const &operator<<(ec_cochannel &in) {
    in.ec_ = post(in.inf_);
    return *this;
  }

  cochannel const &operator>>(ec_cochannel &out) {
    out.ec_ = wait(out.inf_);
    return *this;
  }

public:
  void close() {
    {
      // 加锁
#if DCO_MULT_THREAD
      std::unique_lock<std::mutex> lock(mtx_);
#endif
      stop_ = true;
    }
    // 下面唤醒的都是无法再执行下去的
    while (1 == post_.notify([](const void *) {}))
      ;
    while (1 == wait_.notify([](const void *) {}))
      ;
  }
};

} // namespace dco
