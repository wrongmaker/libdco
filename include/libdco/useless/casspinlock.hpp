#pragma once
#include <atomic>

#include "boost/smart_ptr/detail/spinlock.hpp"

namespace dco {

// class casspinlock {
//  private:
//   std::atomic_flag flag_;  // 这玩意是无锁的

//  public:
//   void lock() {
//     while (flag_.test_and_set(std::memory_order_acquire))
//       ;
//   }

//   void unlock() { flag_.clear(std::memory_order_release); }

//  public:
//   casspinlock(const casspinlock&) = delete;
//   casspinlock& operator=(const casspinlock&) = delete;
//   casspinlock() : flag_{false} {}
// };

// 废弃 使用boost::detail::spinlock替代

}  // namespace dco