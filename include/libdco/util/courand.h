#pragma once
#include <memory>
#include <random>

#include "cosingleton.hpp"

namespace dco {

class courand {
private:
  std::mt19937 rand_;

public:
  unsigned int random(unsigned int max);

public:
  courand(const courand &) = delete;
  courand();
  ~courand();
};

#define dco_rand cosingleton<courand>::instance()

} // namespace dco
