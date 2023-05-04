#include "libdco/hook/cofdctx.h"
#include "libdco/hook/coepoll.h"
#include "libdco/hook/cofdcollect.h"
#include "libdco/hook/cohook_linux.h"
#include "libdco/util/codefer.h"

#include <mutex>
#include <sys/poll.h>
#include <sys/socket.h>

using namespace dco;

cofdctx::cofdctx()
    : fd_(0), events_(0), timeout_con_(0), timeout_send_(0), timeout_recv_(0),
      isblock_(false) {}

bool cofdctx::nonblock_set(bool nonblock) {
  int flags = call_except_eintr<int>(fcntl_ori, fd_, F_GETFL, 0);
  bool old = flags & O_NONBLOCK;
  if (nonblock == old)
    return old;

  call_except_eintr<int>(fcntl_ori, fd_, F_SETFL,
                         nonblock ? (flags | O_NONBLOCK)
                                  : (flags & ~O_NONBLOCK));
  isblock_set(!nonblock);
  return true;
}

void cofdctx::fd_set(int fd) { fd_ = fd; }

void cofdctx::timeout_con_set(uint32_t timeout) { timeout_con_ = timeout; }

void cofdctx::isblock_set(bool isblock) { isblock_ = isblock; }

void cofdctx::timeout_set(int type, uint32_t ms) {
  switch (type) {
  case SO_RCVTIMEO:
    timeout_recv_ = ms;
    break;
  case SO_SNDTIMEO:
    timeout_send_ = ms;
    break;
  }
}

int cofdctx::fd_get() const { return fd_; }

uint32_t cofdctx::timeout_con_get() const { return timeout_con_; }

bool cofdctx::isblock_get() const { return isblock_; }

uint32_t cofdctx::timeout_get(int type) const {
  switch (type) {
  case SO_RCVTIMEO:
    return timeout_recv_;
  case SO_SNDTIMEO:
    return timeout_send_;
  }
  return 0;
}

void cofdctx::reset() {
  fd_ = 0;
  timeout_con_ = 0;
  timeout_send_ = 0;
  timeout_recv_ = 0;
  isblock_ = false;
  in_ptrs_.clear();
  out_ptrs_.clear();
  in_out_ptrs_.clear();
  err_ptrs_.clear();
}

cofdctx::vec_event_ptrs &cofdctx::event_ptrs_get(uint32_t events) {
  if ((events & POLLIN) && (events & POLLOUT)) {
    return err_ptrs_;
  } else if (events & POLLIN) {
    return in_ptrs_;
  } else if (events & POLLOUT) {
    return out_ptrs_;
  }
  return err_ptrs_;
}

// 惊群 可用EPOLLEXCLUSIVE这个标记

bool cofdctx::add(uint32_t events, cofdctx_event_ptr event_ptr) {
  std::lock_guard<std::mutex> lock(mtx_);
  auto &event_ptrs = event_ptrs_get(events);
  event_ptrs.push_back(event_ptr); // 一个fd可以被多次epoll或poll

  uint32_t add_event = events & (POLLIN | POLLOUT);
  if (add_event == 0)
    add_event |= POLLERR;

  uint32_t all_flag_event = events_ | add_event;
  add_event = all_flag_event & ~events_; // 新加的类型判断

  // 不同的events就需要
  if (all_flag_event != events_) {

    do {
      // 尝试加入
      if (all_flag_event == add_event) {
        if (dco_epoll->dco_epoll_add(fd_, all_flag_event))
          break;
      } else if (dco_epoll->dco_epoll_mod(fd_, all_flag_event)) { // 尝试修改
        break;
      }

      // 加入失败 移除刚刚加入的事件
      event_ptrs.pop_back();
      return false;
    } while (0);

    events_ = events;
  }
  return true;
}

void target_all_poll_hook(uint32_t events,
                          cofdctx::vec_event_ptrs &event_ptrs) {
  for (auto &event_ptr : event_ptrs) {
    if (event_ptr->finish_) {
      event_ptr = nullptr;
      continue;
    }
    event_ptr->revents_ = events;
    dco_fdcollect->sche_ptr()->dco_resume(event_ptr->ctx_);
    event_ptr = nullptr;
  }
  event_ptrs.clear();
}

void cofdctx::target(uint32_t events) {
  std::unique_lock<std::mutex> lock(mtx_);
  uint32_t err_events = POLLERR | POLLHUP | POLLNVAL;
  uint32_t not_target_events = 0;

  uint32_t check = POLLIN | err_events;
  if (events & check) {
    target_all_poll_hook(events & check, in_ptrs_);
  } else if (!in_ptrs_.empty()) {
    not_target_events |= POLLIN;
  }

  check = POLLOUT | err_events;
  if (events & check) {
    target_all_poll_hook(events & check, out_ptrs_);
  } else if (!out_ptrs_.empty()) {
    not_target_events |= POLLOUT;
  }

  check = POLLIN | POLLOUT | err_events;
  if (events & check) {
    target_all_poll_hook(events & check, in_out_ptrs_);
  } else if (!in_out_ptrs_.empty()) {
    not_target_events |= POLLIN | POLLOUT;
  }

  check = err_events;
  if (events & check) {
    target_all_poll_hook(events & check, err_ptrs_);
  } else if (!err_ptrs_.empty()) {
    not_target_events |= POLLERR;
  }

  uint32_t del_event = events_ & ~not_target_events;
  (void)del_event;
  if (not_target_events != events_) {
    do {
      // 尝试删除
      if (not_target_events == 0) {
        if (dco_epoll->dco_epoll_del(fd_, not_target_events))
          break;
      } else if (dco_epoll->dco_epoll_mod(fd_, not_target_events)) { // 尝试修改
        break;
      }

      return;
    } while (0);

    events_ = not_target_events;
    return;
  }
}

void cofdctx::close() { target(POLLNVAL); }
