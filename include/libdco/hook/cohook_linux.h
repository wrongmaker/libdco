#pragma once

#include <netdb.h>
#include <poll.h>
#include <resolv.h>
#include <unistd.h>

#include <map>

#include "coerrno.h"

// hook poll
// event->poll->epoll

extern "C" {
typedef int (*socket_t)(int, int, int);
extern socket_t socket_ori;

typedef int (*socketpair_t)(int, int, int, int[2]);
extern socketpair_t socketpair_ori;

typedef int (*connect_t)(int, const struct sockaddr *, socklen_t);
extern connect_t connect_ori;

typedef int (*accept_t)(int, struct sockaddr *, socklen_t *);
extern accept_t accept_ori;

typedef int (*close_t)(int);
extern close_t close_ori;

typedef ssize_t (*read_t)(int, void *, size_t);
extern read_t read_ori;

typedef ssize_t (*write_t)(int, const void *, size_t);
extern write_t write_ori;

typedef ssize_t (*readv_t)(int, const struct iovec *, int);
extern readv_t readv_ori;

typedef ssize_t (*writev_t)(int, const struct iovec *, int);
extern writev_t writev_ori;

typedef ssize_t (*sendto_t)(int, const void *, size_t, int,
                            const struct sockaddr *, socklen_t);
extern sendto_t sendto_ori;

typedef ssize_t (*recvfrom_t)(int, void *, size_t, int, struct sockaddr *,
                              socklen_t *);
extern recvfrom_t recvfrom_ori;

typedef ssize_t (*send_t)(int, const void *, size_t, int);
extern send_t send_ori;

typedef ssize_t (*recv_t)(int, void *, size_t, int);
extern recv_t recv_ori;

typedef ssize_t (*sendmsg_t)(int, const struct msghdr *, int);
extern sendmsg_t sendmsg_ori;

typedef ssize_t (*recvmsg_t)(int, const struct msghdr *, int);
extern recvmsg_t recvmsg_ori;

typedef int (*poll_t)(struct pollfd[], nfds_t, int);
extern poll_t poll_ori;

typedef int (*select_t)(int, fd_set *, fd_set *, fd_set *, struct timeval *);
extern select_t select_ori;

typedef unsigned int (*sleep_t)(unsigned int seconds);
extern sleep_t sleep_ori;

// 毫秒才会执行hook
typedef int (*usleep_t)(useconds_t usec);
extern usleep_t usleep_ori;

typedef int (*setsockopt_t)(int, int, int, const void *, socklen_t);
extern setsockopt_t setsockopt_ori;

typedef int (*fcntl_t)(int, int, ...);
extern fcntl_t fcntl_ori;

typedef int (*ioctl_t)(int, unsigned long int, ...);
extern ioctl_t ioctl_ori;

typedef int (*gethostbyname_r_t)(const char *__restrict name,
                                 struct hostent *__restrict __result_buf,
                                 char *__restrict __buf, size_t __buflen,
                                 struct hostent **__restrict __result,
                                 int *__restrict __h_errnop);
extern gethostbyname_r_t gethostbyname_r_ori;
}

namespace dco {
extern void init_hook();
}
