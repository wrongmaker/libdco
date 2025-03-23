#pragma once
#include <stdint.h>
#include <time.h>
#include <atomic>

namespace dco {

class cotools {
public:
  static time_t ms2until(time_t ms);
  static time_t msnow();
  static bool atomic_decrement_if_positive(std::atomic<int> &sem, int min, int step);
  static bool atomic_increase_if_positive(std::atomic<int> &sem, int max, int step);
};

} // namespace dco
