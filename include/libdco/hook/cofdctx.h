#pragma once

#include "libdco/co/coctx.h"
#include <cstdint>
#include <vector>

namespace dco {

class cofdcollect;
class coepoll;
class cofdctx {
public:
  class cofdctx_event {
  public:
    uint32_t revents_;
    coctx *ctx_;
    bool finish_;

    cofdctx_event() : revents_(0), ctx_(nullptr), finish_(false) {}
  };

  typedef std::shared_ptr<cofdctx_event> cofdctx_event_ptr;
  typedef std::vector<cofdctx_event_ptr> vec_event_ptrs;

private:
  int fd_;
  int events_;
  uint32_t timeout_con_;  // ms
  uint32_t timeout_send_; // ms
  uint32_t timeout_recv_; // ms
  bool isblock_;
  vec_event_ptrs in_ptrs_;
  vec_event_ptrs out_ptrs_;
  vec_event_ptrs in_out_ptrs_;
  vec_event_ptrs err_ptrs_;
  std::mutex mtx_;
  friend class cofdcollect;
  friend class coepoll;

private:
  vec_event_ptrs& event_ptrs_get(uint32_t events);

public:
  cofdctx();

  bool nonblock_set(bool nonblock);

  void fd_set(int fd);
  void timeout_con_set(uint32_t timeout);
  void isblock_set(bool isblock);
  void timeout_set(int type, uint32_t ms);

  int fd_get() const;
  uint32_t timeout_con_get() const;
  bool isblock_get() const;
  uint32_t timeout_get(int type) const;

  void reset();
  bool add(uint32_t events, cofdctx_event_ptr event_ptr);
  void target(uint32_t events);
  void close();
};

typedef std::shared_ptr<cofdctx> cofdctx_ptr;

} // namespace dco
