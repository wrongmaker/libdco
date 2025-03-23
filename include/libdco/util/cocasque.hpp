#pragma once
#include "boost/lockfree/policies.hpp"
#include "boost/lockfree/queue.hpp"
#include <atomic>
#include <queue>
#include <utility>

#define CO_BOOT_CASQUE_RELEASE_AFTER_USED 1

#if CO_BOOT_CASQUE_RELEASE_AFTER_USED
#include "boost/smart_ptr/atomic_shared_ptr.hpp"
#include "boost/smart_ptr/make_shared.hpp"
#endif

#include "concurrentqueue.h"

namespace dco {
#if CO_BOOT_CASQUE_RELEASE_AFTER_USED == 0

template <class Val_> class cocasque {
public:
  struct cascirnode {
    Val_ val_;
    std::atomic<cascirnode *> next_;
    cascirnode() : next_(nullptr), val_() {}
  };

private:
  std::atomic<cascirnode *> head_;
  std::atomic<cascirnode *> tail_;
  std::atomic<cascirnode *> head_free_;
  std::atomic<cascirnode *> tail_free_;

public:
  cocasque() {
    cascirnode *node = new cascirnode();
    head_.store(node);
    tail_.store(node);
    cascirnode *node_free = new cascirnode();
    head_free_.store(node_free);
    tail_free_.store(node_free);
  }
  // 移动构造
  cocasque(cocasque &&m) {
    head_.store(m.head_);
    m.head_ = nullptr;
    tail_.store(m.tail_);
    m.tail_ = nullptr;
    head_free_.store(m.head_free_);
    m.head_free_ = nullptr;
    tail_free_.store(m.tail_free_);
    m.tail_free_ = nullptr;
  }
  cocasque(const cocasque &) = delete;
  cocasque &operator=(const cocasque &) = delete;
  ~cocasque() {
    // 要确保没有在用了在释放
    // 如果在用就释放可能会造成链表未链接完毕
    // 会造成内存泄漏
    while (head_.load()) {
      cascirnode *head = head_.load();
      head_.store(head->next_);
      delete head;
    }
    while (head_free_.load()) {
      cascirnode *head = head_free_.load();
      head_free_.store(head->next_);
      delete head;
    }
  }

private:
  cascirnode *pop_free() {
    cascirnode *head = head_free_.load();
    cascirnode *head_next = nullptr;
    bool end = false;
    for (; !end;) {
      end = true;
      do {
        BOOST_ASSERT(head != nullptr);
        head_next = head->next_.load();
        if (head_next == nullptr) {
          if (head == tail_free_.load()) {
            head = new cascirnode();
            break;
          }
          // 在这一瞬间同一个head的next被置空了,next读不到,但又不是末尾,获取下新的头重新读下
          head = head_free_.load();
          end = false;
          break;
        }
      } while (!head_free_.compare_exchange_weak(head, head_next));
    }
    head->next_.store(nullptr);
    return head;
  }

  void push_free(cascirnode *node) {
    cascirnode *tail = tail_free_.load();
    while (!tail_free_.compare_exchange_weak(tail, node))
      ;
    tail->next_.store(node);
  }

public:
  bool push(const Val_ &val) {
    cascirnode *node = pop_free();
    BOOST_ASSERT(node != nullptr);
    cascirnode *tail = tail_.load();
    while (!tail_.compare_exchange_weak(tail, node))
      ;
    tail->val_ = val;
    tail->next_.store(node);
    return true;
  }

  bool pop(Val_ &val) {
    cascirnode *head = head_.load();
    cascirnode *head_next = nullptr;
    bool end = false;
    for (; !end;) {
      end = true;
      do {
        BOOST_ASSERT(head != nullptr);
        head_next = head->next_.load();
        if (head_next == nullptr) {
          if (head == tail_.load()) {
            return false;
          }
          // 在这一瞬间同一个head的next被置空了,next读不到,但又不是末尾,获取下新的头重新读下
          head = head_.load();
          end = false;
          break;
        }
      } while (!head_.compare_exchange_weak(head, head_next));
    }
    val = head->val_;
    head->next_.store(nullptr);
    push_free(head);
    // 这里直接释放有问题
    // 若这个head还被上面的循环占用
    // 就被释放了就会读到一个错误数据
    // 除非这里改成智能指针的形式
    return true;
  }
};

#else

// 对于make_shared的智能指针
// make_shared会创建一块包括控制块大小的内存
// 所以shared_ptr定义了自己的内存分配代码的时候
// 调用make_shared是无法正常被执行的
// 如果需要用tmalloc这个分配器,需要继承tbuffer
// 需要直接调用new函数生成shared_ptr

// template <class Val_> class cascirnode {
// public:
//   Val_ val_;
//   boost::atomic_shared_ptr<cascirnode<Val_>> next_;
//   cascirnode() : val_(), next_(nullptr) {}
//   ~cascirnode() {}
// };

// template <class Val_> class cocasque {
// private:
//   boost::atomic_shared_ptr<cascirnode<Val_>> head_;
//   boost::atomic_shared_ptr<cascirnode<Val_>> tail_;

// public:
//   cocasque() : head_(nullptr), tail_(nullptr) {
//     boost::shared_ptr<cascirnode<Val_>> node =
//         boost::make_shared<cascirnode<Val_>>();
//     head_.store(node);
//     tail_.store(node);
//   }
//   // 移动构造
//   cocasque(cocasque &&m) {
//     head_.store(m.head_);
//     m.head_ = nullptr;
//     tail_.store(m.tail_);
//     m.tail_ = nullptr;
//   }
//   cocasque(const cocasque &) = delete;
//   cocasque &operator=(const cocasque &) = delete;
//   ~cocasque() {
//     // 要确保没有在用了在释放
//     // 如果在用就释放可能会造成链表未链接完毕
//     // 会造成内存泄漏
//     while (head_.load()) {
//       boost::shared_ptr<cascirnode<Val_>> head = head_.load();
//       head_.store(head->next_);
//       head.reset();
//     }
//   }

// public:
//   bool push(const Val_ &val) {
//     boost::shared_ptr<cascirnode<Val_>> node =
//         boost::make_shared<cascirnode<Val_>>();
//     boost::shared_ptr<cascirnode<Val_>> tail = tail_.load();
//     while (!tail_.compare_exchange_weak(tail, node))
//       ;
//     tail->val_ = val;
//     tail->next_.store(node);
//     return true;
//   }

//   bool pop(Val_ &val) {
//     boost::shared_ptr<cascirnode<Val_>> head = head_.load();
//     boost::shared_ptr<cascirnode<Val_>> head_next = nullptr;
//     bool end = false;
//     do {
//       end = true;
//       do {
//         head_next = head->next_.load();
//         if (head_next == nullptr) {
//           if (head == tail_.load()) {
//             return false;
//           }
//           //
//           在这一瞬间同一个head的next被置空了,next读不到,但又不是末尾,获取下新的头重新读下
//           head = head_.load();
//           end = false;
//           break;
//         }
//       } while (!head_.compare_exchange_weak(head, head_next));
//     } while (!end);
//     val = head->val_;
//     // 等待指针指针自动释放
//     return true;
//   }
// };

// 下面这个实现还是会有删除后读取的情况
// template <class Val_> class cascirnode {
// public:
//   Val_ val_;
//   std::atomic<cascirnode<Val_> *> next_;
//   cascirnode() : val_(), next_(nullptr) {}
//   ~cascirnode() {}
// };

// template <class Val_> class cocasque {
// private:
//   std::atomic<cascirnode<Val_> *> head_;
//   std::atomic<cascirnode<Val_> *> tail_;
//   std::atomic<cascirnode<Val_> *> head_free_;
//   std::atomic<cascirnode<Val_> *> tail_free_;

// public:
//   cocasque() : head_(nullptr), tail_(nullptr) {
//     cascirnode<Val_> *node = new cascirnode<Val_>();
//     head_.store(node);
//     tail_.store(node);
//     cascirnode<Val_> *node_free = new cascirnode<Val_>();
//     head_free_.store(node);
//     tail_free_.store(node);
//   }
//   // 移动构造
//   cocasque(cocasque &&m) {
//     head_.store(m.head_);
//     m.head_ = nullptr;
//     tail_.store(m.tail_);
//     m.tail_ = nullptr;
//     head_free_.store(m.head_free_);
//     m.head_free_ = nullptr;
//     tail_free_.store(m.tail_free_);
//     m.tail_free_ = nullptr;
//   }
//   cocasque(const cocasque &) = delete;
//   cocasque &operator=(const cocasque &) = delete;
//   ~cocasque() {
//     // 要确保没有在用了在释放
//     // 如果在用就释放可能会造成链表未链接完毕
//     // 会造成内存泄漏
//     while (head_.load()) {
//       cascirnode<Val_> *head = head_.load();
//       head_.store(head->next_);
//       delete head;
//     }
//     while (head_free_.load()) {
//       cascirnode<Val_> *head = head_free_.load();
//       head_free_.store(head->next_);
//       delete head;
//     }
//   }

// private:
//   void push_free(cascirnode<Val_> *node) {
//     node->next_.store(nullptr);
//     for (;;) {
//       cascirnode<Val_> *tail = tail_free_.load(std::memory_order_acquire);
//       cascirnode<Val_> *next_ptr =
//       tail->next_.load(std::memory_order_acquire); cascirnode<Val_> *tail2 =
//       tail_free_.load(std::memory_order_acquire);
//       // 快速校验
//       if (__builtin_expect(tail == tail2, 1)) {
//         if (next_ptr == nullptr) {
//           if (tail->next_.compare_exchange_weak(next_ptr, node)) {
//             tail_free_.compare_exchange_strong(tail, node);
//             return;
//           }
//         } else {
//           tail_free_.compare_exchange_strong(tail, next_ptr);
//         }
//       }
//     }
//   }

//   cascirnode<Val_> *pop_free() {
//     for (;;) {
//       cascirnode<Val_> *head = head_free_.load(std::memory_order_acquire);
//       cascirnode<Val_> *next_ptr =
//       head->next_.load(std::memory_order_acquire); cascirnode<Val_> *tail =
//       tail_free_.load(std::memory_order_acquire); cascirnode<Val_> *head2 =
//       head_free_.load(std::memory_order_acquire);
//       // 快速校验
//       if (__builtin_expect(head == head2, 1)) {
//         if (head == tail) {
//           if (next_ptr == nullptr) {
//             cascirnode<Val_> *node = new cascirnode<Val_>();
//             return node;
//           }
//           tail_free_.compare_exchange_strong(tail, next_ptr);
//         } else {
//           if (next_ptr == nullptr)
//             continue;
//           if (head_free_.compare_exchange_weak(head, next_ptr)) {
//             head->next_.store(nullptr);
//             return head;
//           }
//         }
//       }
//     }
//     return nullptr;
//   }

// public:
//   bool push(const Val_ &val) {
//     cascirnode<Val_> *node = pop_free();
//     if (node == nullptr)
//       return false;
//     node->next_.store(nullptr);
//     for (;;) {
//       cascirnode<Val_> *tail = tail_.load(std::memory_order_acquire);
//       cascirnode<Val_> *next_ptr =
//       tail->next_.load(std::memory_order_acquire); cascirnode<Val_> *tail2 =
//       tail_.load(std::memory_order_acquire);
//       // 快速校验
//       if (__builtin_expect(tail == tail2, 1)) {
//         if (next_ptr == nullptr) {
//           if (tail->next_.compare_exchange_weak(next_ptr, node)) {
//             tail_.compare_exchange_strong(tail, node);
//             return true;
//           }
//         } else {
//           tail_.compare_exchange_strong(tail, next_ptr);
//         }
//       }
//     }
//     return false;
//   }

//   bool pop(Val_ &val) {
//     for (;;) {
//       cascirnode<Val_> *head = head_.load(std::memory_order_acquire);
//       cascirnode<Val_> *next_ptr =
//       head->next_.load(std::memory_order_acquire); cascirnode<Val_> *tail =
//       tail_.load(std::memory_order_acquire); cascirnode<Val_> *head2 =
//       head_.load(std::memory_order_acquire);
//       // 快速校验
//       if (__builtin_expect(head == head2, 1)) {
//         if (head == tail) {
//           if (next_ptr == nullptr)
//             return false;
//           tail_.compare_exchange_strong(tail, next_ptr);
//         } else {
//           if (next_ptr == nullptr)
//             continue;
//           val = std::move(next_ptr->val_);
//           if (head_.compare_exchange_weak(head, next_ptr)) {
//             // 将指针塞入缓存
//             push_free(head);
//             return true;
//           }
//         }
//       }
//     }
//     return false;
//   }
// };

// 不继承 析构非虚函数
// template <class Val_> class cocasque {
// private:
//   boost::lockfree::queue<Val_, boost::lockfree::fixed_sized<false>> que_;

// public:
//   cocasque() : que_(1) {}
//   // 移动构造
//   cocasque(cocasque &&m) { m.que_ = std::move(que_); }
//   cocasque(const cocasque &) = delete;
//   cocasque &operator=(const cocasque &) = delete;
//   ~cocasque() {}

// public:
//   bool push(const Val_ &val) { return que_.push(val); }

//   bool pop(Val_ &val) { return que_.pop(val); }
// };

// template <class T>
// using cocasque = boost::lockfree::queue<T,
// boost::lockfree::fixed_sized<false>>;

template <class T> class cocasque {
public:
  cocasque() {}
  ~cocasque() {}

public:
  bool push(const T &val) { return que_.enqueue(val); }

  bool pop(T &val) { return que_.try_dequeue(val); }
  
  moodycamel::ConcurrentQueue<T> &que() { return que_; }

private:
  moodycamel::ConcurrentQueue<T> que_;
};

#endif

} // namespace dco
