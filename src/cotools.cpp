#include "libdco/util/cotools.h"

#include <chrono>

using namespace dco;
using namespace std::chrono;

#define tools_ms_now                                                           \
  duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count()

time_t cotools::ms2until(time_t ms) { return tools_ms_now + ms; }

time_t cotools::msnow() { return tools_ms_now; }

bool cotools::atomic_decrement_if_positive(std::atomic<int> &sem, int min, int step) {
  int current = sem.load();
  for (;;) {
    if (current <= 0)
      return false;
    if (sem.compare_exchange_weak(current, current - step))
      return true;
  }
}

bool cotools::atomic_increase_if_positive(std::atomic<int> &sem, int max, int step) {
  int current = sem.load();
  for (;;) {
    if (current >= max)
      return false;
    if (sem.compare_exchange_weak(current, current + step))
      return true;
  }
}
