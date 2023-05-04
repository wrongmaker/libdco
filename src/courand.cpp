#include "libdco/util/courand.h"

using namespace dco;

unsigned int courand::random(unsigned int max) {
  std::uniform_int_distribution<> dist(0, max);
  return dist(rand_);
}

courand::courand() : rand_(std::random_device{}()) {}

courand::~courand() {}