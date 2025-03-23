// #pragma once
// #include <list>

// #include "libdco/co/cochannel.hpp"
// #include "libdco/co/cosemaphore.h"
// #include "libdco/util/cotools.h"

// namespace dco {

// class coselect {
// public:
//   class cocaseopt {
//   private:
//     friend class coselect;

//     enum opt_type { opt_wait, opt_post };

//   private:
//     int code_;
//     opt_type opt_;
//     cochannel_base *base_;
//     cowaitque_mt::waitnode node_;
//     std::function<void(int)> func_;

//   public:
//     cocaseopt(opt_type opt, cochannel_base *base, coctx *ctx,
//               const void *payload, const std::function<void(int)> &func)
//         : code_(0), opt_(opt), base_(base), node_(ctx, payload), func_(func) {}
//     ~cocaseopt() {}
//   };

// private:
//   int flag_;
//   time_t ms_until_;
//   coschedule *ptr_sche_;
//   std::function<void()> func_default_;
//   std::function<void(int)> func_timeout_;
//   std::list<cocaseopt> cases_;

// private:
//   void case_node_remove();
//   bool case_node_check(bool hook);
//   bool case_node_exec();
//   void run();

// public:
//   template <class T, int N>
//   coselect &wait_case(cochannel<T, N> *chan, T &t,
//                       const std::function<void(int)> &func) {
//     cases_.emplace_back(cocaseopt::opt_wait,
//                         static_cast<cochannel_base *>(chan),
//                         ptr_sche_->dco_curr_ctx(), std::addressof(t), func);
//     return *this;
//   }

//   template <class T, int N>
//   coselect &wait_case(cochannel<T, N> &chan, T &t,
//                       const std::function<void(int)> &func) {
//     return wait_case(std::addressof(chan), t, func);
//   }

//   template <class T, int N>
//   coselect &post_case(cochannel<T, N> *chan, const T &t,
//                       const std::function<void(int)> &func) {
//     cases_.emplace_back(cocaseopt::opt_post,
//                         static_cast<cochannel_base *>(chan),
//                         ptr_sche_->dco_curr_ctx(), std::addressof(t), func);
//     return *this;
//   }

//   template <class T, int N>
//   coselect &post_case(cochannel<T, N> &chan, const T &t,
//                       const std::function<void(int)> &func) {
//     return post_case(std::addressof(chan), t, func);
//   }

// //   // 毫秒
// //   coselect &timeout_case(time_t ms, const std::function<void(int)> &func) {
// //     ms_until_ = cotools::ms2until(ms);
// //     func_timeout_ = func;
// //     return *this;
// //   }

//   coselect &default_case(const std::function<void()> &func) {
//     func_default_ = func;
//     return *this;
//   }

// public:
//   coselect(const coselect &) = delete;
//   coselect(coschedule *);
//   ~coselect();
// };

// } // namespace dco
