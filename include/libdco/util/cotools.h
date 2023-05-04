#pragma once
#include <stdint.h>
#include <time.h>

namespace dco {

class cotools {
public:
  static time_t ms2until(time_t ms);
  static time_t msnow();
};

} // namespace dco
