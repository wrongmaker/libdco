#pragma once
#include <sys/socket.h>

namespace dco {
class cosocket {
public:
  cosocket(int fd) : fd_(fd) {}
  virtual ~cosocket() {}

public:
  virtual size_t write(const void *buff, size_t len) {
    return ::send(fd_, buff, len, 0);
  }
  virtual size_t read(void *buff, size_t len) {
    return ::recv(fd_, buff, len, 0);
  }

private:
  int fd_;
};
} // namespace dco
