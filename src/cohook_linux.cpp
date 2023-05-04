#include "libdco/hook/cohook_linux.h"
#include "libdco/co/coschedule.h"

#include <cstddef>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <resolv.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "libdco/co/comutex.h"
#include "libdco/hook/cofdcollect.h"
#include "libdco/hook/cofdctx.h"
#include "libdco/util/cotools.h"

using namespace dco;

extern "C" {
socket_t socket_ori = NULL;
socketpair_t socketpair_ori = NULL;
connect_t connect_ori = NULL;
accept_t accept_ori = NULL;
close_t close_ori = NULL;
read_t read_ori = NULL;
write_t write_ori = NULL;
readv_t readv_ori = NULL;
writev_t writev_ori = NULL;
sendto_t sendto_ori = NULL;
recvfrom_t recvfrom_ori = NULL;
send_t send_ori = NULL;
recv_t recv_ori = NULL;
sendmsg_t sendmsg_ori = NULL;
recvmsg_t recvmsg_ori = NULL;
poll_t poll_ori = NULL;
select_t select_ori = NULL;
sleep_t sleep_ori = NULL;
usleep_t usleep_ori = NULL;
setsockopt_t setsockopt_ori = NULL;
fcntl_t fcntl_ori = NULL;
ioctl_t ioctl_ori = NULL;
gethostbyname_r_t gethostbyname_r_ori = NULL;
}

void dco::init_hook() {
  if (socket_ori == NULL) {
    socket_ori = (socket_t)dlsym(RTLD_NEXT, "socket");
    socketpair_ori = (socketpair_t)dlsym(RTLD_NEXT, "socketpair");
    connect_ori = (connect_t)dlsym(RTLD_NEXT, "connect");
    accept_ori = (accept_t)dlsym(RTLD_NEXT, "accept");
    close_ori = (close_t)dlsym(RTLD_NEXT, "close");
    read_ori = (read_t)dlsym(RTLD_NEXT, "read");
    write_ori = (write_t)dlsym(RTLD_NEXT, "write");
    readv_ori = (readv_t)dlsym(RTLD_NEXT, "readv");
    writev_ori = (writev_t)dlsym(RTLD_NEXT, "writev");
    sendto_ori = (sendto_t)dlsym(RTLD_NEXT, "sendto");
    recvfrom_ori = (recvfrom_t)dlsym(RTLD_NEXT, "recvfrom");
    send_ori = (send_t)dlsym(RTLD_NEXT, "send");
    recv_ori = (recv_t)dlsym(RTLD_NEXT, "recv");
    sendmsg_ori = (sendmsg_t)dlsym(RTLD_NEXT, "sendmsg");
    recvmsg_ori = (recvmsg_t)dlsym(RTLD_NEXT, "recvmsg");
    poll_ori = (poll_t)dlsym(RTLD_NEXT, "poll");
    select_ori = (select_t)dlsym(RTLD_NEXT, "select");
    sleep_ori = (sleep_t)dlsym(RTLD_NEXT, "sleep");
    usleep_ori = (usleep_t)dlsym(RTLD_NEXT, "usleep");
    setsockopt_ori = (setsockopt_t)dlsym(RTLD_NEXT, "setsockopt");
    fcntl_ori = (fcntl_t)dlsym(RTLD_NEXT, "fcntl");
    ioctl_ori = (ioctl_t)dlsym(RTLD_NEXT, "ioctl");
    gethostbyname_r_ori =
        (gethostbyname_r_t)dlsym(RTLD_NEXT, "gethostbyname_r");
  }
}

/**
 * 进程间通讯 管道相关函数
 * pipe函数族
 * dup函数族
 *
 */

class dco_hook {
public:
  static coctx *co_curr_ctx_get() {
    if (dco_fdcollect == nullptr)
      return nullptr;
    return dco_fdcollect->sche_ptr()->dco_curr_ctx();
  }

  static bool co_fdcollect_check() { return dco_fdcollect != nullptr; }

  static coschedule *co_sche() { return dco_fdcollect->sche_ptr(); }
};

class NoBlockGurad {
private:
  bool set_noblock_ = false;
  cofdctx_ptr fdctx_ = nullptr;

public:
  NoBlockGurad(int fd, int flags) {
    if ((flags & MSG_DONTWAIT) == 0 || dco_hook::co_curr_ctx_get() == nullptr)
      return;
    fdctx_ = dco_fdcollect->get(fd);
    if (fdctx_ == nullptr || !fdctx_->isblock_get())
      return;
    set_noblock_ = true;
    fdctx_->isblock_set(false);
    dco_fd_debug("NoBlockGurad fd:{} not block set", fd);
  }
  ~NoBlockGurad() {
    if (set_noblock_ == false || fdctx_ == nullptr)
      return;
    fdctx_->isblock_set(true);
    dco_fd_debug("NoBlockGurad fd:{} not block roll back", fdctx_->fd_get());
  }
};

static thread_local std::shared_ptr<dco::comutex> ptr_dns_mtx = nullptr;

inline int co_pool_nfds_1(coctx *ctx, struct pollfd *fds, nfds_t nfds, int ms) {
  cofdctx::cofdctx_event_ptr event_ptr = nullptr;
  auto call = [&fds, &nfds, &event_ptr](coctx *ctx) {
    bool added = false;
    do {
      pollfd &pfd = fds[0];
      pfd.revents = 0; // 清空返回时间
      if (pfd.fd < 0)
        break;

      event_ptr = std::make_shared<cofdctx::cofdctx_event>();
      event_ptr->ctx_ = ctx;

      auto fdctx = dco_fdcollect->get(pfd.fd);
      if (fdctx == nullptr || !fdctx->add(pfd.events, event_ptr)) {
        pfd.revents = POLLNVAL;
        event_ptr = nullptr;
        break;
      }

      added = true;
    } while (0);

    if (!added) {
      // 全部fd都无法加入epoll
      if (event_ptr != nullptr) {
        fds[0].revents = event_ptr->revents_;
        event_ptr->finish_ = true;
      }
      // 给他唤醒
      dco_hook::co_sche()->dco_resume(ctx);
    }
  };

  // 设置挂起后call
  if (ms > 0)
    dco_hook::co_sche()->dco_sleep(ctx, ms, call);
  else
    dco_hook::co_sche()->dco_yield(ctx, call);

  int n = 0;
  if (event_ptr != nullptr) {
    fds[0].revents = event_ptr->revents_;
    event_ptr->finish_ = true;
  }
  if (fds[0].revents)
    ++n;
  return n;
}

inline int co_pool_nfds_n(coctx *ctx, struct pollfd *fds, nfds_t nfds, int ms) {
  std::vector<cofdctx::cofdctx_event_ptr> paddings(nfds);
  auto call = [&fds, &nfds, &paddings](coctx *ctx) {
    bool added = false;
    for (nfds_t i = 0; i < nfds; ++i) {
      pollfd &pfd = fds[i];
      pfd.revents = 0; // 清空返回时间
      if (pfd.fd < 0)
        continue;

      paddings[i] = std::make_shared<cofdctx::cofdctx_event>();
      paddings[i]->ctx_ = ctx;

      auto fdctx = dco_fdcollect->get(pfd.fd);
      if (fdctx == nullptr || !fdctx->add(pfd.events, paddings[i])) {
        pfd.revents = POLLNVAL;
        paddings[i] = nullptr;
        continue;
      }

      added = true;
    }

    if (!added) {
      // 全部fd都无法加入epoll
      for (nfds_t i = 0; i < nfds; ++i) {
        if (paddings[i] != nullptr) {
          fds[i].revents = paddings[i]->revents_;
          paddings[i]->finish_ = true;
        }
      }
      // 给他唤醒
      dco_hook::co_sche()->dco_resume(ctx);
    }
  };

  // 设置挂起后call
  if (ms > 0)
    dco_hook::co_sche()->dco_sleep(ctx, ms, call);
  else
    dco_hook::co_sche()->dco_yield(ctx, call);

  int n = 0;
  for (nfds_t i = 0; i < nfds; ++i) {
    if (paddings[i] != nullptr) {
      fds[i].revents = paddings[i]->revents_;
      paddings[i]->finish_ = true;
    }
    if (fds[i].revents)
      ++n;
  }
  return n;
}

inline int co_poll(struct pollfd *fds, nfds_t nfds, int ms,
                   bool nonblocking_check) {
  coctx *ctx = dco_hook::co_curr_ctx_get();
  if (ctx == nullptr || ms == 0)
    return poll_ori(fds, nfds, ms);

  // 全部是负数fd时, 等价于sleep
  nfds_t negative_fd = 0;
  for (nfds_t i = 0; i < nfds; ++i) {
    if (fds[i].fd < 0)
      ++negative_fd;
  }

  if (nfds == negative_fd) {
    if (ms > 0)
      dco_hook::co_sche()->dco_sleep(ctx, ms);
    return 0;
  }

  if (nonblocking_check) {
    // 执行一次非阻塞的poll, 检测异常或无效fd.
    // 同时检查是否有已经就绪了的fd
    int res = poll_ori(fds, nfds, 0);
    if (res != 0) {
      return res;
    }
  }

  int n = 0;
  if (nfds == 1)
    n = co_pool_nfds_1(ctx, fds, nfds, ms);
  else
    n = co_pool_nfds_n(ctx, fds, nfds, ms);

  errno = 0;
  return n;
}

template <typename func_ori, typename... func_args>
static ssize_t co_epoll_in_out(int fd, func_ori fn, const char *fn_name,
                               uint32_t event, int timeout_so,
                               func_args &&...args) {

  dco_fd_debug("co add {} fd:{} events:{:x}", fn_name, fd, event);

  coctx *ctx = dco_hook::co_curr_ctx_get();
  if (ctx == nullptr)
    return fn(fd, std::forward<func_args>(args)...);

  cofdctx_ptr fdctx = dco_fdcollect->get(fd);
  if (fdctx == nullptr || !fdctx->isblock_get())
    return fn(fd, std::forward<func_args>(args)...);

  uint32_t timeout = fdctx->timeout_get(timeout_so);
  int poll_timeout = timeout == 0 ? -1 : timeout;

  struct pollfd fds;
  fds.fd = fd;
  fds.events = event;
  fds.revents = 0;

  for (;;) {
    int triggers = co_poll(&fds, 1, poll_timeout, true);
    if (-1 == triggers) {
      if (errno == EINTR)
        continue;
      return -1;
    } else if (0 == triggers) { // poll等待超时
      errno = EAGAIN;
      return -1;
    } else {
      break;
    }
  }

  ssize_t res;
  for (;;) {
    res = fn(fd, std::forward<func_args>(args)...);
    if (res == -1) {
      if (errno == EINTR)
        continue;
      return -1;
    } else {
      break;
    }
  }

  return res;
}

int socket(int domain, int type, int protocol) {
  if (socket_ori == NULL)
    dco::init_hook();

  int sock = socket_ori(domain, type, protocol);
  if (sock >= 0) {
    dco_fdcollect->create(sock, true);
  }
  dco_fd_debug("co socket fd:{}", sock);

  return sock;
}

int socketpair(int domain, int type, int protocol, int sv[2]) {
  if (socketpair_ori == NULL)
    dco::init_hook();

  int res = socketpair_ori(domain, type, protocol, sv);
  if (res == 0) {
    dco_fdcollect->create(sv[0], true);
    dco_fdcollect->create(sv[1], true);
  }
  return res;
}

int connect(int fd, const struct sockaddr *addr, socklen_t addrlen) {
  if (connect_ori == NULL)
    dco::init_hook();

  coctx *ctx = dco_hook::co_curr_ctx_get();
  if (ctx == nullptr)
    return connect_ori(fd, addr, addrlen);

  cofdctx_ptr fdctx = dco_fdcollect->get(fd);
  if (!fdctx)
    return connect_ori(fd, addr, addrlen);

  if (!fdctx->isblock_get())
    return connect_ori(fd, addr, addrlen);

  int res;

  {
    bool isblock = fdctx->isblock_get();
    if (isblock)
      fdctx->nonblock_set(true);
    res = connect_ori(fd, addr, addrlen);
    if (isblock)
      fdctx->nonblock_set(false);
  }

  if (res == 0) {
    return 0;
  } else if (res == -1 && errno == EINPROGRESS) {
    // poll wait
  } else {
    return res;
  }

  // EINPROGRESS. use poll for wait connect complete.
  time_t before = 0;
  uint32_t con_timeout = fdctx->timeout_con_get();
  int poll_timeout = (con_timeout == 0) ? -1 : con_timeout;
  if (con_timeout != 0)
    before = cotools::msnow();

  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = POLLOUT;
  int triggers = co_poll(&pfd, 1, poll_timeout, false);
  if (triggers <= 0 || pfd.revents != POLLOUT) {
    if (poll_timeout != -1 && cotools::msnow() - before >= poll_timeout)
      errno = ETIMEDOUT;
    else
      errno = ECONNREFUSED;
    return -1;
  }

  int sock_err = 0;
  socklen_t len = sizeof(int);
  if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &sock_err, &len))
    return -1;

  if (!sock_err)
    return 0;

  errno = sock_err;
  return -1;
}

int close(int fd) {
  if (close_ori == NULL)
    dco::init_hook();

  dco_fd_debug("co close fd:{}", fd);
  dco_fdcollect->del(fd);
  return close_ori(fd);
}

int __close(int fd) {
  if (close_ori == NULL)
    dco::init_hook();

  dco_fd_debug("co __close fd:{}", fd);
  dco_fdcollect->del(fd);
  return close_ori(fd);
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
  if (accept_ori == NULL)
    dco::init_hook();

  cofdctx_ptr fdctx = dco_fdcollect->get(sockfd);
  if (!fdctx) {
    errno = EBADF;
    return -1;
  }

  int sock = co_epoll_in_out(sockfd, accept_ori, __func__, POLLIN, SO_RCVTIMEO,
                             addr, addrlen);
  if (sock >= 0) {
    dco_fdcollect->create(sock, true);
  }
  dco_fd_debug("co accept fd:{}", sock);
  return sock;
}

ssize_t write(int fd, const void *buf, size_t count) {
  if (write_ori == NULL)
    dco::init_hook();

  return co_epoll_in_out(fd, write_ori, __func__, POLLOUT, SO_SNDTIMEO, buf,
                         count);
}

ssize_t read(int fd, void *buf, size_t count) {
  if (read_ori == NULL)
    dco::init_hook();

  return co_epoll_in_out(fd, read_ori, __func__, POLLIN, SO_RCVTIMEO, buf,
                         count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
  if (writev_ori == NULL)
    dco::init_hook();

  return co_epoll_in_out(fd, writev_ori, __func__, POLLOUT, SO_SNDTIMEO, iov,
                         iovcnt);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
  if (readv_ori == NULL)
    dco::init_hook();

  return co_epoll_in_out(fd, readv_ori, __func__, POLLIN, SO_RCVTIMEO, iov,
                         iovcnt);
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen) {
  if (sendto_ori == NULL)
    dco::init_hook();

  NoBlockGurad gurad(sockfd, flags);
  return co_epoll_in_out(sockfd, sendto_ori, __func__, POLLOUT, SO_SNDTIMEO,
                         buf, len, flags, dest_addr, addrlen);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen) {
  if (recvfrom_ori == NULL)
    dco::init_hook();

  NoBlockGurad gurad(sockfd, flags);
  return co_epoll_in_out(sockfd, recvfrom_ori, __func__, POLLIN, SO_RCVTIMEO,
                         buf, len, flags, src_addr, addrlen);
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
  if (send_ori == NULL)
    dco::init_hook();

  NoBlockGurad gurad(sockfd, flags);
  return co_epoll_in_out(sockfd, send_ori, __func__, POLLOUT, SO_SNDTIMEO, buf,
                         len, flags);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
  if (recv_ori == NULL)
    dco::init_hook();

  NoBlockGurad gurad(sockfd, flags);
  return co_epoll_in_out(sockfd, recv_ori, __func__, POLLIN, SO_RCVTIMEO, buf,
                         len, flags);
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags) {
  if (sendmsg_ori == NULL)
    dco::init_hook();

  return co_epoll_in_out(sockfd, sendmsg_ori, __func__, POLLOUT, SO_SNDTIMEO,
                         msg, flags);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
  if (recvmsg_ori == NULL)
    dco::init_hook();

  return co_epoll_in_out(sockfd, recvmsg_ori, __func__, POLLIN, SO_RCVTIMEO,
                         msg, flags);
}

int poll(struct pollfd *fds, nfds_t nfds, int timeout) {
  if (poll_ori == NULL)
    dco::init_hook();

  return co_poll(fds, nfds, timeout, true);
}

int __poll(struct pollfd *fds, nfds_t nfds, int timeout) {
  if (poll_ori == NULL)
    dco::init_hook();

  return co_poll(fds, nfds, timeout, true);
}

int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
           struct timeval *timeout) {
  if (select_ori == NULL)
    dco::init_hook();

  int timeout_ms = -1;
  if (timeout)
    timeout_ms = timeout->tv_sec * 1000 + timeout->tv_usec / 1000;

  dco_fd_debug("co select nfds:{} rd_set:{} wr_set:{} er_set:{} timeout_ms:{}",
               (int)nfds, (void *)readfds, (void *)writefds, (void *)exceptfds,
               timeout_ms);

  coctx *ctx = dco_hook::co_curr_ctx_get();
  if (ctx == nullptr || timeout_ms == 0)
    return select_ori(nfds, readfds, writefds, exceptfds, timeout);

  if (!nfds) {
    dco_hook::co_sche()->dco_sleep(ctx, timeout_ms);
    return 0;
  }

  nfds = std::min<int>(nfds, FD_SETSIZE);

  // 执行一次非阻塞的select, 检测异常或无效fd.
  fd_set rfs, wfs, efs;
  FD_ZERO(&rfs);
  FD_ZERO(&wfs);
  FD_ZERO(&efs);
  if (readfds)
    rfs = *readfds;
  if (writefds)
    wfs = *writefds;
  if (exceptfds)
    efs = *exceptfds;
  timeval zero_tv = {0, 0};
  int n =
      select_ori(nfds, (readfds ? &rfs : nullptr), (writefds ? &wfs : nullptr),
                 (exceptfds ? &efs : nullptr), &zero_tv);
  if (n != 0) {
    if (readfds)
      *readfds = rfs;
    if (writefds)
      *writefds = wfs;
    if (exceptfds)
      *exceptfds = efs;
    return n;
  }

  // convert fd_set to pollfd, and clear 3 fd_set.
  std::pair<fd_set *, uint32_t> sets[3] = {
      {readfds, POLLIN}, {writefds, POLLOUT}, {exceptfds, 0}};

  std::map<int, int> pfd_map;
  for (int i = 0; i < 3; ++i) {
    fd_set *fds = sets[i].first;
    if (!fds)
      continue;
    int event = sets[i].second;
    for (int fd = 0; fd < nfds; ++fd) {
      if (FD_ISSET(fd, fds)) {
        pfd_map[fd] |= event;
      }
    }
    FD_ZERO(fds);
  }

  std::vector<pollfd> pfds(pfd_map.size());
  int i = 0;
  for (auto &kv : pfd_map) {
    pollfd &pfd = pfds[i++];
    pfd.fd = kv.first;
    pfd.events = kv.second;
  }

  // poll
  n = co_poll(pfds.data(), pfds.size(), timeout_ms, true);
  if (n <= 0)
    return n;

  // convert pollfd to fd_set.
  int ret = 0;
  for (size_t i = 0; i < pfds.size(); ++i) {
    pollfd &pfd = pfds[i];
    if (pfd.revents & POLLIN) {
      if (readfds) {
        FD_SET(pfd.fd, readfds);
        ++ret;
      }
    }

    if (pfd.revents & POLLOUT) {
      if (writefds) {
        FD_SET(pfd.fd, writefds);
        ++ret;
      }
    }

    if (pfd.revents & ~(POLLIN | POLLOUT)) {
      if (exceptfds) {
        FD_SET(pfd.fd, exceptfds);
        ++ret;
      }
    }
  }

  return ret;
}

int gethostbyname_r(const char *__restrict name,
                    struct hostent *__restrict __result_buf,
                    char *__restrict __buf, size_t __buflen,
                    struct hostent **__restrict __result,
                    int *__restrict __h_errnop) {
  if (gethostbyname_r_ori == NULL)
    dco::init_hook();

  if (!dco_hook::co_fdcollect_check()) {
    return gethostbyname_r_ori(name, __result_buf, __buf, __buflen, __result,
                               __h_errnop);
  }
  if (ptr_dns_mtx == nullptr) {
    ptr_dns_mtx = std::make_shared<dco::comutex>(dco_hook::co_sche());
  }
  // thread mtx lock
  std::unique_lock<dco::comutex> lock(*ptr_dns_mtx.get());
  return gethostbyname_r_ori(name, __result_buf, __buf, __buflen, __result,
                             __h_errnop);
}

unsigned int sleep(unsigned int seconds) {
  if (sleep_ori == NULL)
    dco::init_hook();

  coctx *ctx = dco_hook::co_curr_ctx_get();
  if (ctx == nullptr)
    return sleep_ori(seconds);

  dco_fd_debug("co sleep ms:{}", seconds * 1000);
  dco_hook::co_sche()->dco_sleep(ctx, seconds * 1000);
  return 0;
}

int usleep(useconds_t usec) {
  if (usleep_ori == NULL)
    dco::init_hook();

  coctx *ctx = dco_hook::co_curr_ctx_get();
  if (ctx == nullptr)
    return usleep_ori(usec);

  uint32_t ms = usec / 1000;
  uint32_t us = usec % 1000;

  dco_fd_debug("co usleep ms:{} us:{}", ms, us);
  if (ms > 0)
    dco_hook::co_sche()->dco_sleep(ctx, ms);

  if (us > 0)
    usleep_ori(us);
  return 0;
}

int setsockopt(int sockfd, int level, int optname, const void *optval,
               socklen_t optlen) {
  if (setsockopt_ori == NULL)
    dco::init_hook();

  dco_fd_debug("setsockopt fd:{} level:{} optname:{}", sockfd, level, optname);

  int res = setsockopt_ori(sockfd, level, optname, optval, optlen);

  if (res == 0 && level == SOL_SOCKET &&
      (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO)) {
    auto fdctx = dco_fdcollect->get(sockfd);
    if (fdctx) {
      const timeval &tv = *(const timeval *)optval;
      int ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;
      fdctx->timeout_set(optname, ms);
    }
  }

  return res;
}

int fcntl(int fd, int cmd, ...) {
  if (fcntl_ori == NULL)
    dco::init_hook();

  dco_fd_debug("fcntl fd:{} cmd:{}", fd, cmd);

  int ret = 0;

  va_list arg_list;
  va_start(arg_list, cmd);

  switch (cmd) {
  // int
  case F_DUPFD:
  case F_DUPFD_CLOEXEC: {
    int arg = va_arg(arg_list, int);

    ret = fcntl_ori(fd, cmd, arg);
    break;
  }

  // int
  case F_SETFD:
  case F_SETOWN:

#ifdef __USE_GNU
  case F_SETSIG:
  case F_SETLEASE:
  case F_NOTIFY:
#endif

#if defined(F_SETPIPE_SZ)
  case F_SETPIPE_SZ:
#endif
  {
    int arg = va_arg(arg_list, int);

    ret = fcntl_ori(fd, cmd, arg);
    break;
  }

  // int
  case F_SETFL: {
    int flags = va_arg(arg_list, int);

    auto fdctx = dco_fdcollect->get(fd);
    if (fdctx) {
      bool isnonblock = !!(flags & O_NONBLOCK);
      fdctx->isblock_set(!isnonblock);
    }

    ret = fcntl_ori(fd, cmd, flags);
    break;
  }

  // struct flock*
  case F_GETLK:
  case F_SETLK:
  case F_SETLKW: {
    struct flock *arg = va_arg(arg_list, struct flock *);

    ret = fcntl_ori(fd, cmd, arg);
    break;
  }

  // struct f_owner_ex*
#ifdef __USE_GNU
  case F_GETOWN_EX:
  case F_SETOWN_EX: {
    struct f_owner_exlock *arg = va_arg(arg_list, struct f_owner_exlock *);

    ret = fcntl_ori(fd, cmd, arg);
    break;
  }
#endif

  // void
  case F_GETFL: {
    ret = fcntl_ori(fd, cmd);
    break;
  }

  // void
  case F_GETFD:
  case F_GETOWN:

#ifdef __USE_GNU
  case F_GETSIG:
  case F_GETLEASE:
#endif

#if defined(F_GETPIPE_SZ)
  case F_GETPIPE_SZ:
#endif
  default: {
    ret = fcntl_ori(fd, cmd);
    break;
  }
  }

  va_end(arg_list);
  return ret;
}

int ioctl(int fd, unsigned long int request, ...) {
  if (ioctl_ori != NULL)
    dco::init_hook();

  dco_fd_debug("ioctl fd:{}", fd);

  va_list va;
  va_start(va, request);
  void *arg = va_arg(va, void *);
  va_end(va);

  if (FIONBIO == request) {
    auto fdctx = dco_fdcollect->get(fd);
    if (fdctx) {
      bool isnonblock = !!*(int *)arg;
      fdctx->isblock_set(!isnonblock);
    }
  }

  return ioctl_ori(fd, request, arg);
}
