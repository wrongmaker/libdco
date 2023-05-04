#pragma once

#include "libdco/all.h"
#include <arpa/inet.h>
#include <exception>
#include <functional>
#include <memory>
#include <netinet/in.h>
#include <sys/socket.h>

namespace dco {
namespace costream {

class conetexc : public std::exception {
public:
  enum errcode { ec_socket, ec_bind, ec_listen, ec_accpet, ec_connect };

public:
  errcode ec_;
  int code_;
  const char *msg_;

public:
  conetexc(errcode ec, int code, const char *msg)
      : ec_(ec), code_(code), msg_(msg) {}
};

typedef std::function<void(int fd)> cofdcall;
class coaccpet {
public:
  coaccpet(const std::string &host, unsigned short port, const cofdcall &call)
      : accpet_call_(call), stop_(false) {
    bzero((char *)&svr_addr_, sizeof(svr_addr_));
    svr_addr_.sin_family = AF_INET;
    inet_pton(AF_INET, host.c_str(), &svr_addr_.sin_addr);
    svr_addr_.sin_port = htons(port);
  }
  virtual ~coaccpet() {}

public:
  void try_accept() {
    // get svr fd
    int svr_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr_fd < 0) {
      throw conetexc{conetexc::ec_socket, svr_fd,
                     "error establishing the server socket"};
    }

    // bind to fd
    int ret = bind(svr_fd, (struct sockaddr *)&svr_addr_, sizeof(svr_addr_));
    if (ret < 0) {
      throw conetexc{conetexc::ec_bind, ret,
                     "error binding socket to server address"};
    }

    // listen
    ret = listen(svr_fd, std::thread::hardware_concurrency());
    if (ret < 0) {
      throw conetexc{conetexc::ec_bind, ret,
                     "error listen socket to server address"};
    }

    // init cli fd
    sockaddr_in cli_addr;
    socklen_t cli_addr_size = sizeof(cli_addr);

    // do accept
    for (; !stop_;) {
      int cli_fd = accept(svr_fd, (sockaddr *)&cli_addr, &cli_addr_size);
      if (cli_fd < 0) {
        throw conetexc{conetexc::ec_accpet, cli_fd,
                       "error accepting request from client"};
      }
      // accept and call
      accpet_call_(cli_fd);
    }
  }

private:
  sockaddr_in svr_addr_;
  cofdcall accpet_call_;
  bool stop_;
};
class coconnect {
public:
  coconnect(const std::string &host, unsigned short port, const cofdcall &call)
      : connect_call_(call) {
    bzero((char *)&remote_addr_, sizeof(remote_addr_));
    remote_addr_.sin_family = AF_INET;
    inet_pton(AF_INET, host.c_str(), &remote_addr_.sin_addr);
    remote_addr_.sin_port = htons(port);
  }
  virtual ~coconnect() {}

public:
  void try_connect() {
    // get remote fd
    int remote_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (remote_fd < 0) {
      throw conetexc{conetexc::ec_socket, remote_fd,
                     "error establishing the remote socket"};
      exit(0);
    }

    // connect to remote
    int ret = connect(remote_fd, (struct sockaddr *)&remote_addr_,
                      sizeof(remote_addr_));
    if (ret < 0) {
      throw conetexc{conetexc::ec_connect, ret,
                     "error connect socket to remote address"};
    }

    // connect and call
    connect_call_(remote_fd);
  }

private:
  sockaddr_in remote_addr_;
  cofdcall connect_call_;
};
} // namespace costream
} // namespace dco
