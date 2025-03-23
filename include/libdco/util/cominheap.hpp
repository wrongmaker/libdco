#pragma once
#include "boost/smart_ptr/shared_ptr.hpp"
#include <cstddef>
#include <cstdio>
#include <functional>
#include <mutex>
#include <vector>

namespace dco {

// 实现一个最小堆
template <class T> class cominheap {
private:
  std::vector<T *> heap_;

public:
  cominheap() {}
  cominheap(const cominheap &) = delete;
  cominheap &operator=(const cominheap &) = delete;
  cominheap(cominheap &&m) : heap_(std::move(m.heap_)) { m.heap_.clear(); }
  virtual ~cominheap() {}

private:
  void swap(size_t a, size_t b) {
    size_t index = heap_[a]->index_;
    heap_[a]->index_ = heap_[b]->index_;
    heap_[b]->index_ = index;
    T *c = heap_[a];
    heap_[a] = heap_[b];
    heap_[b] = c;
  }

  bool comp(T *a, T *b) { return *a < *b; }

  void repair_front(size_t loc) {
    size_t loc1 = (loc << 1) + 1;
    while (loc1 < heap_.size()) {
      if (loc1 + 1 < heap_.size() && comp(heap_[loc1 + 1], heap_[loc1]))
        loc1 += 1;
      if (!comp(heap_[loc1], heap_[loc]))
        break;
      swap(loc, loc1);
      loc = loc1;
      loc1 = (loc << 1) + 1;
    }
  }

  void repair_back(size_t loc) {
    while (loc > 0) {
      size_t loc1 = (loc - 1) >> 1;
      if (comp(heap_[loc1], heap_[loc]))
        break;
      swap(loc, loc1);
      loc = loc1;
    }
  }

protected:
  inline bool in_remove(T *node) {
    if (heap_.empty() || node->index_ >= heap_.size())
      return false;
    size_t index = node->index_;
    BOOST_ASSERT(heap_[index] == node);
    size_t max_index = heap_.size() - 1;
    if (index == max_index) {
      heap_[index]->index_ = (size_t)-1;
      heap_.pop_back();
    } else {
      swap(index, max_index);
      heap_[max_index]->index_ = (size_t)-1;
      heap_.pop_back();
      if (index > 0 && comp(heap_[index], heap_[(index - 1) >> 1]))
        repair_back(index);
      else
        repair_front(index);
    }
    return true;
  }

  inline T *in_top() {
    if (heap_.empty())
      return nullptr;
    return heap_[0];
  }

public:
  virtual void push(T *node) {
    node->index_ = heap_.size();
    heap_.push_back(node);
    repair_back(heap_.size() - 1);
  }

  virtual void pop() {
    if (heap_.empty())
      return;
    size_t max_index = heap_.size() - 1;
    swap(0, max_index);
    heap_[max_index]->index_ = (size_t)-1;
    heap_.pop_back();
    repair_front(0);
  }

  virtual bool remove(T *node) { return in_remove(node); }

  virtual T *top() { return in_top(); }

  virtual T *try_pop(const std::function<bool(T *)> &pop_check) {
    T *node = in_top();
    if (node == nullptr || !pop_check(node))
      return nullptr;
    in_remove(node);
    return node;
  }

  virtual bool empty() { return heap_.empty(); }

  virtual size_t size() { return heap_.size(); }
};

// 实现一个最小堆
template <class T> class cominheap_mt : public cominheap<T> {
private:
  std::mutex mtx_;

public:
  void push(T *node) {
    std::lock_guard<std::mutex> lock{mtx_};
    cominheap<T>::push(node);
  }

  void pop() {
    std::lock_guard<std::mutex> lock{mtx_};
    cominheap<T>::pop();
  }

  bool remove(T *node) {
    if (node->index_ == (size_t)-1)
      return false;
    std::lock_guard<std::mutex> lock{mtx_};
    return cominheap<T>::in_remove(node);
  }

  T *top() {
    std::lock_guard<std::mutex> lock{mtx_};
    return cominheap<T>::in_top();
  }

  T *try_pop(const std::function<bool(T *)> &pop_check) {
    if (cominheap<T>::empty())
      return nullptr;
    std::lock_guard<std::mutex> lock{mtx_};
    return cominheap<T>::try_pop(pop_check);
  }

  bool empty() {
    std::lock_guard<std::mutex> lock{mtx_};
    return cominheap<T>::empty();
  }
};

} // namespace dco
