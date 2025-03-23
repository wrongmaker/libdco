#pragma once
#include "libdco/co/cowaitque.h"
#include "libdco/util/coringbuffer.hpp"
#include "libdco/util/cotools.h"
#include <functional>
#include <memory>

namespace dco {
class coselect;
class cochannel_base {
public:
  // virtual void wait_remove(cowaitque_mt::waitnode &node) = 0;
  // virtual void post_remove(cowaitque_mt::waitnode &node) = 0;
  // virtual int wait_check(cowaitque_mt::waitnode &node, bool hook = true) = 0;
  // virtual int post_check(cowaitque_mt::waitnode &node, bool hook = true) = 0;
};

template <class Val_, int N> class cochannel : public cochannel_base {
private:
  coschedule *ptr_sche_;
  cowaitque_mt wait_;
  cowaitque_mt post_;
  volatile bool stop_;
  dco::coringbuffer_mt<Val_, N> buf_que_;

private:
  friend class coselect;

public:
  cochannel() = delete;
  cochannel(const cochannel &) = delete;
  cochannel(coschedule *sche)
      : wait_(sche), post_(sche), ptr_sche_(sche), stop_(false) {}
  ~cochannel() {}

public:
  struct ec_cochannel {
    int ec_;
    Val_ &inf_;
    ec_cochannel(Val_ &inf) : ec_(0), inf_(inf) {}
  };

private:
  inline bool check_from_buff() { return !buf_que_.empty() || !post_.empty(); }
  inline bool check_to_buff() { return !buf_que_.full() || !wait_.empty(); }
  inline int wait_check_real(Val_ &val) {
    // 先尝试从buff中拷贝
    if (buf_que_.pop(val)) {
      post_.call(); // 唤醒一个post
      return 1;
    }
    // 再尝试从post中拷贝
    auto call = [&val](const void *payload) { val = *(Val_ *)payload; };
    if (1 == post_.call(call))
      return 1;
    return -1;
  }
  inline int post_check_real(const Val_ &val) {
    // 先尝试拷贝到wait
    auto call = [&val](const void *payload) { *(Val_ *)payload = val; };
    if (1 == wait_.call(call))
      return 1;
    // 再尝试拷贝到buff
    if (buf_que_.push(val))
      return 1;
    return -1;
  }
  inline bool copy_from_buff_call(void *val) {
    return wait_check_real(*(Val_ *)val) == 1;
  }
  inline bool copy_to_buff_call(void *val) {
    return post_check_real(*(const Val_ *)val) == 1;
  }

public:
  // -1:失败,1:成功
  int try_wait(Val_ &val) { return wait_check_real(std::addressof(val)); }

  // -2:关闭,-1:失败,1:成功
  int wait(Val_ &val) {
    if (1 == wait_check_real(val))
      return 1;
    return wait_.wait(std::addressof(val),
                      std::bind(&cochannel<Val_, N>::check_from_buff, this),
                      std::bind(&cochannel<Val_, N>::copy_from_buff_call, this,
                                std::placeholders::_1));
  }

  // -2:关闭,0:超时,-1:失败,1:成功
  int wait_for(time_t ms, Val_ &val) {
    if (1 == wait_check_real(val))
      return 1;
    return wait_.wait_for(ms, std::addressof(val),
                          std::bind(&cochannel<Val_, N>::check_from_buff, this),
                          std::bind(&cochannel<Val_, N>::copy_from_buff_call,
                                    this, std::placeholders::_1));
  }

  // -1:失败,1:成功
  int try_post(const Val_ &val) { return post_check_real(std::addressof(val)); }

  // -2:关闭,-1:失败,1:成功
  int post(const Val_ &val) {
    if (1 == post_check_real(val))
      return 1;
    return post_.wait(std::addressof(const_cast<Val_ &>(val)),
                      std::bind(&cochannel<Val_, N>::check_to_buff, this),
                      std::bind(&cochannel<Val_, N>::copy_to_buff_call, this,
                                std::placeholders::_1));
  }

  // -2:关闭,0:超时,-1:失败,1:成功
  int post_for(time_t ms, Val_ &val) {
    if (1 == post_check_real(val))
      return 1;
    return post_.wait_for(ms, std::addressof(const_cast<Val_ &>(val)),
                          std::bind(&cochannel<Val_, N>::check_to_buff, this),
                          std::bind(&cochannel<Val_, N>::copy_to_buff_call,
                                    this, std::placeholders::_1));
  }

  int operator<<(const Val_ &val) { return post(val); }

  int operator>>(Val_ &val) { return wait(val); }

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
    post_.close();
    wait_.close();
  }
};

} // namespace dco
