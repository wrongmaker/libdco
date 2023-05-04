#pragma once

#include "libdco/co/all.h"
// #include "libdco/fmt/all.h"
#include "libdco/hook/all.h"
#include "libdco/util/all.h"
#include "spdlog/spdlog.h"

// 可以生成多个协程调度器
// 以下定义仅仅支持单利调度模式下调用

// 调用这个宏初始化单例协程调度器
#if DCO_MULT_THREAD
#define dco_init_worker(num)                                                   \
  {                                                                            \
    dco::cosingleton<dco::coschedule>::init(                                   \
        std::make_shared<dco::coschedule>(num));                               \
    dco::cosingleton<dco::courand>::init(std::make_shared<dco::courand>());    \
    dco::cosingleton<dco::cofdcollect>::init(                                  \
        std::make_shared<dco::cofdcollect>(__dco_sche_ptr));                   \
    dco::cosingleton<dco::coepoll>::init(std::make_shared<dco::coepoll>());    \
    spdlog::set_pattern("%^[%x %H:%M:%S.%e][%L][tid:%t]%v%$");                 \
  }

#define dco_init() dco_init_worker(std::thread::hardware_concurrency())
#else
#define dco_init(...)                                                          \
  {                                                                            \
    dco::cosingleton<dco::coschedule>::init(                                   \
        std::make_shared<dco::coschedule>());                                  \
    dco::cosingleton<dco::courand>::init(std::make_shared<dco::courand>());    \
    dco::cosingleton<dco::cofdcollect>::init(                                  \
        std::make_shared<dco::cofdcollect>(__dco_sche_ptr));                   \
    dco::cosingleton<dco::coepoll>::init(std::make_shared<dco::coepoll>());    \
    spdlog::set_pattern("%^[%x %H:%M:%S.%e][%L][tid:%t]%v%$");                 \
  }
#endif

// 若协程调度非单例形式存在无法调用以下宏
#define dco_epoll_run() dco_epoll->run()

#define dco_sche __dco_sche

#define dco_sche_ptr __dco_sche_ptr

#define dco_go dco::dco_cogo() =

#define dco_await dco::coawait() =

// #define dco_yield dco_sche->dco_yield(dco_sche->dco_curr_ctx())

#define dco_yield dco::dco_coyield() = 

#define dco_gosche dco_sche->dco_gosche(dco_sche->dco_curr_ctx())

#define dco_sleep(ms) dco_sche->dco_sleep(dco_sche->dco_curr_ctx(), (ms))

#define dco_resume(ctx) dco_sche->dco_resume(ctx)

#define dco_thread_id dco_sche->dco_thread_id()

using dco_sem = dco::dco_cosem;

template <class T> using dco_chan = dco::dco_cochan<T>;

using dco_mutex = dco::dco_comutex;

using dco_mutex_seq = dco::dco_comutex_seq;

using dco_mutex_shared = dco::dco_comutex_shared;

using dco_cond = dco::dco_cocond;

template <class T> using dco_object = dco::dco_coobject<T>;

template <class Main_, class Func_>
using dco_gen = dco::dco_cogen<Main_, Func_>;

template <class Main_, class Func_>
using dco_gen_yield = dco::cogenerator_yield<Main_, Func_>;

// chan select 操作
// chan select case 错误码 -2:关闭 0:超时 1:获取
#define dco_select dco::coselect(dco_sche_ptr)

#define dco_make_variable_with_line__(prefix, line) prefix##line
#define dco_make_variable_with_line_(prefix, line)                             \
  dco_make_variable_with_line__(prefix, line)
#define dco_make_variable_with_line(prefix)                                    \
  dco_make_variable_with_line_(prefix, __LINE__)

#define dco_defer                                                              \
  dco::codefer dco_make_variable_with_line(__dco_def);                         \
  dco_make_variable_with_line(__dco_def) =
#define dco_defer_t dco::codefer

#define dco_defer_scope_exit dco::defer_scope __dco_defer_scope_exit__
#define dco_defer_scope __dco_defer_scope_exit__ =
#define dco_defer_scope_invoke __dco_defer_scope_exit__.last_invoke()
#define dco_defer_scope_cancel __dco_defer_scope_exit__.last_cancel()

#if DCO_DISABLE_WRITE_LOG
#define dco_debug(fmt, args...)
#define dco_info(fmt, args...)
#define dco_warn(fmt, args...)
#define dco_error(fmt, args...)
#else
#define dco_debug(fmt, args...)                                                \
  spdlog::debug("[cid:{}] " #fmt,                                              \
                dco_sche->dco_curr_ctx() ? dco_sche->dco_curr_ctx()->id() : 0, \
                ##args)
#define dco_info(fmt, args...)                                                 \
  spdlog::info("[cid:{}] " #fmt,                                               \
               dco_sche->dco_curr_ctx() ? dco_sche->dco_curr_ctx()->id() : 0,  \
               ##args)
#define dco_warn(fmt, args...)                                                 \
  spdlog::warn("[cid:{}] " #fmt,                                               \
               dco_sche->dco_curr_ctx() ? dco_sche->dco_curr_ctx()->id() : 0,  \
               ##args)
#define dco_error(fmt, args...)                                                \
  spdlog::error("[cid:{}] " #fmt,                                              \
                dco_sche->dco_curr_ctx() ? dco_sche->dco_curr_ctx()->id() : 0, \
                ##args)
#endif
