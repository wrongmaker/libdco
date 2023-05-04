#pragma once

#include <memory>
#include <shared_mutex>
#include <unordered_map>

#include "libdco/co/coctx.h"
#include "libdco/hook/cofdctx.h"
#include "libdco/util/cosingleton.hpp"
#include "spdlog/spdlog.h"

namespace dco {
template <typename R, typename F, typename... Args>
static R call_except_eintr(F f, Args &&...args) {
  R res;
  for (;;) {
    res = f(std::forward<Args>(args)...);
    if (res != -1 || errno != EINTR)
      break;
  }
  return res;
}

class cofdcollect {
public:
  class cofdpack {
  private:
    cofdctx_ptr fdctx_;
    mutable boost::detail::spinlock mtx_;
    friend class cofdcollect;

  public:
    cofdpack() : fdctx_(nullptr), mtx_ BOOST_DETAIL_SPINLOCK_INIT {}
  };
  typedef std::shared_ptr<cofdpack> cofdpack_ptr;

private:
  static const int kBucketBit = 10;
  static const int kBucketMask = (1 << kBucketBit) - 1;

  class cofdbucket {
  private:
    typedef std::unordered_map<int, cofdpack_ptr> cofdpack_map;
    cofdpack_map packs_;
    std::shared_timed_mutex mtx_;
    friend class cofdcollect;

  public:
    cofdbucket() {}
  };

  cofdbucket buckets_[kBucketMask];
  coschedule *ptr_sche_;

public:
  cofdcollect(coschedule *sche);
  cofdcollect(const cofdcollect &) = delete;
  ~cofdcollect();

private:
  cofdpack_ptr get_insert(int fd, bool insert = false);

public:
  cofdctx_ptr get(int fd);
  void del(int fd);
  void create(int fd, bool block);
  coschedule *sche_ptr() const;
};

#define dco_fdcollect dco::cosingleton<dco::cofdcollect>::instance()

#if DCO_DISABLE_WRITE_LOG
#define dco_fd_debug(fmt, args...)
#define dco_fd_info(fmt, args...)
#define dco_fd_warn(fmt, args...)
#define dco_fd_error(fmt, args...)
#else
#define dco_fd_debug(fmt, args...)                                             \
  spdlog::debug("[cid:{}] " #fmt,                                              \
                dco_fdcollect->sche_ptr()->dco_curr_ctx()                      \
                    ? dco_fdcollect->sche_ptr()->dco_curr_ctx()->id()          \
                    : 0,                                                       \
                ##args)
#define dco_fd_info(fmt, args...)                                              \
  spdlog::info("[cid:{}] " #fmt,                                               \
               dco_fdcollect->sche_ptr()->dco_curr_ctx()                       \
                   ? dco_fdcollect->sche_ptr()->dco_curr_ctx()->id()           \
                   : 0,                                                        \
               ##args)
#define dco_fd_warn(fmt, args...)                                              \
  spdlog::warn("[cid:{}] " #fmt,                                               \
               dco_fdcollect->sche_ptr()->dco_curr_ctx()                       \
                   ? dco_fdcollect->sche_ptr()->dco_curr_ctx()->id()           \
                   : 0,                                                        \
               ##args)
#define dco_fd_error(fmt, args...)                                             \
  spdlog::error("[cid:{}] " #fmt,                                              \
                dco_fdcollect->sche_ptr()->dco_curr_ctx()                      \
                    ? dco_fdcollect->sche_ptr()->dco_curr_ctx()->id()          \
                    : 0,                                                       \
                ##args)
#endif

} // namespace dco
