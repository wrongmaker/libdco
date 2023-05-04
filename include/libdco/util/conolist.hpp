#pragma once

namespace dco {

// 实现一个链表连接器
template <class T> class conolist {
private:
  T *head_;
  T *tail_;

public:
  conolist() : head_(nullptr), tail_(nullptr) {}
  conolist(conolist &&m) : head_(m.head_), tail_(m.tail_) {
    m.head_ = nullptr;
    m.tail_ = nullptr;
  }
  conolist(const conolist &) = delete;
  conolist &operator=(const conolist &) = delete;

  T *front() { return head_; }

  void pop_front() { // front
    if (!head_)
      return;
    BOOST_ASSERT(tail_ != nullptr);
    T *node = head_;
    head_ = (T *)head_->next_;
    if (head_ == nullptr)
      tail_ = nullptr;
    else
      head_->prev_ = nullptr;
    node->prev_ = nullptr;
    node->next_ = nullptr;
  }

  void push_back(T *node) {
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

  void remove(T *node) {
    if (!node)
      return;
    // if (!node->prev_ && !node->next_ && head_ != node) // 没有位置链接
    // 可能是头部
    //     return;
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
      // BOOST_ASSERT(node->prev_ != nullptr);
      // BOOST_ASSERT(node->next_ != nullptr);
      if (node->prev_)
        node->prev_->next_ = node->next_;
      if (node->next_)
        node->next_->prev_ = node->prev_;
    }
    node->prev_ = nullptr;
    node->next_ = nullptr;
  }
};

} // namespace dco
