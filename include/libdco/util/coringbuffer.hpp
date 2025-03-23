#pragma once

#include <array>
#include <mutex>

#include "boost/smart_ptr/detail/spinlock.hpp"

namespace dco {

template <class Val_, int N> class coringbuffer {
private:
  std::array<Val_, N> buffer_; // 底层存储
  std::size_t head_;           // 队列头索引
  std::size_t tail_;           // 队列尾索引
  bool full_;                  // 队列是否已满

public:
  coringbuffer() : head_(0), tail_(0), full_(false) {}
  coringbuffer(coringbuffer &) = delete;
  virtual ~coringbuffer() {}

protected:
  bool in_empty() const { return !full_ && (head_ == tail_); }
  bool in_full() const { return full_; }

public:
  bool push(const Val_ &value) {
    if (in_full())
      return false;
    buffer_[tail_] = value;
    if (full_)
      head_ = (head_ + 1) % N; // 覆盖头部
    tail_ = (tail_ + 1) % N;
    full_ = (tail_ == head_);
    return true;
  }

  bool pop(Val_ &value) {
    if (in_empty())
      return false;
    value = std::move(buffer_[head_]);
    head_ = (head_ + 1) % N;
    full_ = false;
    return true;
  }

  bool empty() const { return in_empty(); }

  bool full() const { return in_full(); }
};

template <class Val_> class coringbuffer<Val_, 0> {
public:
  coringbuffer() {}
  coringbuffer(coringbuffer &) = delete;
  virtual ~coringbuffer() {}

public:
  bool push(const Val_ &value) { return false; }

  bool pop(Val_ &value) { return false; }

  bool empty() const { return true; }

  bool full() const { return true; }
};

template <class Val_, int N>
class coringbuffer_mt : public coringbuffer<Val_, N> {
private:
  mutable boost::detail::spinlock mtx_;

public:
  coringbuffer_mt() : mtx_ BOOST_DETAIL_SPINLOCK_INIT {}

public:
  bool push(const Val_ &value) {
    std::lock_guard<boost::detail::spinlock> lock{mtx_};
    return coringbuffer<Val_, N>::push(value);
  }

  bool pop(Val_ &value) {
    std::lock_guard<boost::detail::spinlock> lock{mtx_};
    return coringbuffer<Val_, N>::pop(value);
  }

  bool empty() const {
    std::lock_guard<boost::detail::spinlock> lock{mtx_};
    return coringbuffer<Val_, N>::in_empty();
  }

  bool full() const {
    std::lock_guard<boost::detail::spinlock> lock{mtx_};
    return coringbuffer<Val_, N>::in_full();
  }
};

template <class Val_> class coringbuffer_mt<Val_, 0> {
public:
  coringbuffer_mt() {}
  coringbuffer_mt(coringbuffer_mt &) = delete;
  virtual ~coringbuffer_mt() {}

public:
  bool push(const Val_ &value) { return false; }

  bool pop(Val_ &value) { return false; }

  bool empty() const { return true; }

  bool full() const { return true; }
};

} // namespace dco
