#pragma once

#include "libdco/co/coschedule.h"
#include "libdco/util/cosingleton.hpp"
#include <sys/epoll.h>

namespace dco {

class coepoll {
private:
  int epoll_fd_;

public:
  coepoll();

private:
  void do_event(epoll_event &event);

public:
  bool dco_epoll_add(int fd, uint32_t events);
  bool dco_epoll_mod(int fd, uint32_t events);
  bool dco_epoll_del(int fd, uint32_t events);
  int do_epoll_create();
  void run();
};

#define dco_epoll dco::cosingleton<dco::coepoll>::instance()

} // namespace dco
