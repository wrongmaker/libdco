#pragma once
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

public:
  void push(T *node) {
    node->index_ = heap_.size();
    heap_.push_back(node);
    repair_back(heap_.size() - 1);
  }

  void pop() {
    if (heap_.empty())
      return;
    swap(0, heap_.size() - 1);
    heap_[heap_.size() - 1]->index_ = (size_t)-1;
    heap_.pop_back();
    repair_front(0);
  }

  bool remove(T *node) {
    if (heap_.empty() || node->index_ >= heap_.size())
      return false;
    size_t index = node->index_;
    if (index == heap_.size() - 1) {
      heap_[heap_.size() - 1]->index_ = (size_t)-1;
      heap_.pop_back();
    } else {
      swap(index, heap_.size() - 1);
      heap_[heap_.size() - 1]->index_ = (size_t)-1;
      heap_.pop_back();
      if (index > 0 && comp(heap_[index], heap_[(index - 1) >> 1]))
        repair_back(index);
      else
        repair_front(index);
    }
    return true;
  }

  T *top() {
    if (heap_.empty())
      return nullptr;
    return heap_[0];
  }

  bool empty() { return heap_.empty(); }

  size_t size() { return heap_.size(); }
};

} // namespace dco
