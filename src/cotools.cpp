#include "libdco/util/cotools.h"

#include <chrono>

using namespace dco;
using namespace std::chrono;

#define tools_ms_now                                                           \
  duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count()

time_t cotools::ms2until(time_t ms) { return tools_ms_now + ms; }

time_t cotools::msnow() { return tools_ms_now; }