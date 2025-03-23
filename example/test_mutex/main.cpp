#include "libdco/all.h"
#include <chrono>
#include <shared_mutex>

int main() {
  dco_init_worker(4);

  dco_mutex mtx;
  for (int i = 0; i < 2; ++i) {
    dco_go[&mtx, i] {
      for (int j = 0; j < 100000; ++j) {
        {
          std::lock_guard<dco_mutex> lock(mtx);
          printf("lock 0 - %d\n", i);
          // dco_sleep(100);
          printf("unlock 0 - %d\n", i);
        }
        dco_swout;
      }
    };
  }

  for (int i = 0; i < 4; ++i) {
    dco_go[&mtx, i](dco::coctx * ctx) {
      for (int j = 0; j < 100000; ++j) {
        {
          std::lock_guard<dco_mutex> lock(mtx);
          printf("lock 1 - %d\n", i);
          // dco_sleep(200);
          printf("unlock 1 - %d\n", i);
        }
        dco_swout;
      }
    };
  }

  // example main loop
  dco_epoll_run();
  return 0;
}
