// #include "libdco/co/coselect.h"

// #include "libdco/util/courand.h"

// using namespace dco;

// coselect::coselect(coschedule *sche)
//     : flag_(0), ms_until_(0), ptr_sche_(sche), func_default_(nullptr),
//       func_timeout_(nullptr) {}

// coselect::~coselect() { run(); }

// void coselect::case_node_remove() {
//   for (auto &one_case : cases_) {
//     one_case.base_->wait_remove(one_case.node_);
//   }
// }

// // check所有case
// bool coselect::case_node_check(bool hook) {
//   bool ret = false;
//   for (auto &one_case : cases_) {
//     if (one_case.opt_ == cocaseopt::opt_wait)
//       one_case.code_ = one_case.base_->wait_check(one_case.node_, hook);
//     else if (one_case.opt_ == cocaseopt::opt_post)
//       one_case.code_ = one_case.base_->post_check(one_case.node_, hook);
//     else
//       continue;
//     if (!hook) {
//       if (one_case.node_.unpack_)
//         one_case.code_ = 1;
//       if (one_case.code_ != 0) {
//         one_case.func_(one_case.code_);
//         ret = true;
//       }
//     }
//   }
//   return ret;
// }

// bool coselect::case_node_exec() {
//   for (auto &one_case : cases_) {
//     if (one_case.node_.unpack_)
//       one_case.code_ = 1;
//     if (one_case.code_ != 0) {
//       one_case.func_(one_case.code_);
//     }
//   }
//   return false;
// }

// // 被唤醒的没执行到
// // 导致出了bug
// void coselect::run() {
//   // 检查是否允许执行
//   BOOST_ASSERT(!cases_.empty() || func_default_ != nullptr ||
//                func_timeout_ != nullptr);
//   // 先跑一边是否可以执行
//   if (case_node_check(false)) {
//     return;
//   }
//   // 如果第一遍不能执行检查下是否有default
//   if (func_default_ != nullptr) {
//     func_default_();
//     return;
//   }
//   // 执行case操作
//   int ret = 1;
//   auto call = [this](coctx *) { case_node_check(true); };
//   // 检查是否有timeout
//   bool istimeout = false;
//   coctx *ctx = ptr_sche_->dco_curr_ctx();
//   if (func_timeout_ != nullptr) {
//     int sleep_res = ptr_sche_->dco_sleep_until(ctx, ms_until_, call);
//     // 失败
//     if (0 > sleep_res)
//       return;
//     // 超时
//     if (0 == sleep_res)
//       ret = 0;
//     istimeout = true;
//   } else {
//     // 失败
//     if (0 != ptr_sche_->dco_yield(ctx, call)) {
//       BOOST_ASSERT(false); // 通知挂起失败
//       return;
//     }
//   }
//   case_node_remove(); // 清理等待节点
//   if (case_node_exec())
//     return;
//   if (istimeout)
//     func_timeout_(ret);
// }