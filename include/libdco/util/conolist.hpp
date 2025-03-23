#pragma once

#include "boost/smart_ptr/detail/spinlock_gcc_atomic.hpp"
#include <atomic>
#include <functional>
#include <mutex>

namespace dco {

template <class T> class conolist {
public:
  virtual ~conolist() {}

public:
  virtual bool pop(const std::function<bool(T *)> &call) = 0;
  virtual void push(T *node) = 0;
  virtual void remove(T *node) = 0;
  virtual bool empty() = 0;
};

// 实现一个链表连接器
template <class T> class conolist_st : public conolist<T> {
private:
  T *head_;
  T *tail_;

public:
  conolist_st() : head_(nullptr), tail_(nullptr) {}
  conolist_st(conolist_st &&m) : head_(m.head_), tail_(m.tail_) {
    m.head_ = nullptr;
    m.tail_ = nullptr;
  }
  conolist_st(const conolist_st &) = delete;
  conolist_st &operator=(const conolist_st &) = delete;
  virtual ~conolist_st() {}

  virtual bool pop(const std::function<bool(T *)> &call) {
    if (!head_)
      return false;
    BOOST_ASSERT(tail_ != nullptr);
    T *node = head_;
    head_ = (T *)head_->next_;
    if (head_ == nullptr)
      tail_ = nullptr;
    else
      head_->prev_ = nullptr;
    node->prev_ = nullptr;
    node->next_ = nullptr;
    if (call == nullptr)
      return true;
    return call(node);
  }

  virtual void push(T *node) {
    if (!node)
      return;
    node->next_ = nullptr;
    node->prev_ = tail_;
    if (tail_ == nullptr)
      head_ = node;
    else
      tail_->next_ = node;
    tail_ = node;
  }

  virtual void remove(T *node) {
    if (!node)
      return;
    if (node == head_) {
      BOOST_ASSERT(head_->next_ == node->next_);
      head_ = (T *)head_->next_;
      if (head_ == nullptr)
        tail_ = nullptr;
      else
        head_->prev_ = nullptr;
    } else if (node == tail_) {
      BOOST_ASSERT(tail_->prev_ == node->prev_);
      tail_ = (T *)tail_->prev_;
      if (tail_ == nullptr)
        head_ = nullptr;
      else
        tail_->next_ = nullptr;
    } else {
      if (node->prev_)
        node->prev_->next_ = node->next_;
      if (node->next_)
        node->next_->prev_ = node->prev_;
    }
    node->prev_ = nullptr;
    node->next_ = nullptr;
  }

  virtual bool empty() { return head_ == nullptr; }
};

template <class T> class conolist_mt : public conolist_st<T> {
private:
  mutable boost::detail::spinlock mtx_;

public:
  conolist_mt() : mtx_ BOOST_DETAIL_SPINLOCK_INIT {}
  conolist_mt(const conolist_mt &) = delete;
  conolist_mt &operator=(const conolist_mt &) = delete;

  virtual bool pop(const std::function<bool(T *)> &call) {
    // pop后调用需要在这个锁里面完成，防止pop操作的时候同时remove造成的操作异常
    std::lock_guard<boost::detail::spinlock> lock{mtx_};
    return conolist_st<T>::pop(call);
  }

  virtual void push(T *node) {
    std::lock_guard<boost::detail::spinlock> lock{mtx_};
    conolist_st<T>::push(node);
  }

  virtual void remove(T *node) {
    std::lock_guard<boost::detail::spinlock> lock{mtx_};
    conolist_st<T>::remove(node);
  }

  virtual bool empty() {
    std::lock_guard<boost::detail::spinlock> lock{mtx_};
    return conolist_st<T>::empty();
  }
};

} // namespace dco