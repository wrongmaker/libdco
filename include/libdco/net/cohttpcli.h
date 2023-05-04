#pragma once

#include <string>
#include <unordered_map>

namespace dco {
class cohttpcli_base {
private:
public:
  virtual void connect() = 0;

public:
  cohttpcli_base(const std::string &host, const int port);
};

class cohttpcli : public cohttpcli_base {
private:
public:
};

class cohttpcli_ssl : public cohttpcli_base {
private:
public:
};
} // namespace dco