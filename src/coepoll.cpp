#include "libdco/hook/coepoll.h"

#include <sys/epoll.h>
#include <sys/poll.h>

#include "libdco/hook/cofdcollect.h"

using namespace dco;

static uint32_t poll_2_epoll_event(short int event) {
  uint32_t out_event = 0;
  if (event & POLLIN)
    out_event |= EPOLLIN;
  if (event & POLLOUT)
    out_event |= EPOLLOUT;
  if (event & POLLERR)
    out_event |= EPOLLERR;
  if (event & POLLHUP)
    out_event |= EPOLLHUP;
  return out_event;
}

static short epoll_2_poll_event(uint32_t event) {
  short int out_event = 0;
  if (event & EPOLLIN)
    out_event |= POLLIN;
  if (event & EPOLLOUT)
    out_event |= POLLOUT;
  if (event & EPOLLERR)
    out_event |= POLLERR;
  if (event & EPOLLHUP)
    out_event |= POLLHUP;
  return out_event;
}

coepoll::coepoll() : epoll_fd_(do_epoll_create()) {}

bool coepoll::dco_epoll_add(int fd, uint32_t events) {
  uint32_t set_events = poll_2_epoll_event(events) | EPOLLET;
  epoll_event ep_event;
  ep_event.events = set_events;
  ep_event.data.fd = fd;
  int res = call_except_eintr<int>(::epoll_ctl, epoll_fd_, EPOLL_CTL_ADD, fd,
                                   &ep_event);
  dco_fd_debug("co epoll add fd:{} events:{:x} res:{}", fd, set_events, res);
  return res == 0;
}

bool coepoll::dco_epoll_mod(int fd, uint32_t events) {
  uint32_t set_events = poll_2_epoll_event(events) | EPOLLET;
  epoll_event ep_event;
  ep_event.events = set_events;
  ep_event.data.fd = fd;
  int res = call_except_eintr<int>(::epoll_ctl, epoll_fd_, EPOLL_CTL_MOD, fd,
                                   &ep_event);
  dco_fd_debug("co epoll mod fd:{} events:{:x} res:{}", fd, set_events, res);
  return res == 0;
}

bool coepoll::dco_epoll_del(int fd, uint32_t events) {
  uint32_t set_events = poll_2_epoll_event(events) | EPOLLET;
  epoll_event ep_event;
  ep_event.events = set_events;
  ep_event.data.fd = fd;
  int res = call_except_eintr<int>(::epoll_ctl, epoll_fd_, EPOLL_CTL_DEL, fd,
                                   &ep_event);
  dco_fd_debug("co epoll del fd:{} events:{:x} res:{}", fd, set_events, res);
  return res == 0;
}

static const int kEpollCount = 1024;

int coepoll::do_epoll_create() {
  int fd = epoll_create(kEpollCount);
  if (fd == -1) {
    exit(-1);
  }
  return fd;
}

void coepoll::do_event(epoll_event &event) {
  auto ptr = dco_fdcollect->get(event.data.fd);
  if (ptr == nullptr) {
    dco_fd_debug("co epoll do event fail, fd:{} events:{:x}",
                 (int)event.data.fd, (uint32_t)event.events);
    return;
  }

  ptr->target(epoll_2_poll_event(event.events));
}

void coepoll::run() {
  for (;;) {
    static epoll_event events[kEpollCount];
    int num_events = ::epoll_wait(epoll_fd_, events, 256, -1); // wait forever

    dco_fd_debug("co epoll_wait, size:{}", num_events);
    for (int i = 0; i < num_events; ++i) {
      // 处理事件
      do_event(events[i]);
    }
  }
}

// errno为thread_local
