#include "libdco/all.h"
#include <chrono>
#include <shared_mutex>

int main() {
  dco_init_worker(4);

  dco_mutex_shared rwmutex;

  dco_go[&rwmutex] {
    for (;;) {
      {
        std::lock_guard<dco_mutex_shared> lock(rwmutex);
        printf("w lock 0\n");
        dco_sleep(100);
        printf("w unlock 0\n");
      }
      // dco_sleep(1);
    }
  };

  dco_go[&rwmutex] {
    for (;;) {
      {
        std::lock_guard<dco_mutex_shared> lock(rwmutex);
        printf("w lock 1\n");
        dco_sleep(100);
        printf("w unlock 1\n");
      }
      // dco_sleep(1);
    }
  };

  dco_go[&rwmutex](dco::coctx * ctx) {
    for (;;) {
      {
        std::shared_lock<dco_mutex_shared> lock(rwmutex);
        printf("r lock 0\n");
        dco_sleep(200);
        printf("r unlock 0\n");
      }
      // dco_sleep(1);
    }
  };

  dco_go[&rwmutex](dco::coctx * ctx) {
    for (;;) {
      {
        std::shared_lock<dco_mutex_shared> lock(rwmutex);
        printf("r lock 1\n");
        dco_sleep(200);
        printf("r unlock 1\n");
      }
      // dco_sleep(1);
    }
  };

  dco_go[&rwmutex](dco::coctx * ctx) {
    for (;;) {
      {
        std::shared_lock<dco_mutex_shared> lock(rwmutex);
        printf("r lock 2\n");
        dco_sleep(200);
        printf("r unlock 2\n");
      }
      // dco_sleep(1);
    }
  };

  dco_go[&rwmutex](dco::coctx * ctx) {
    for (;;) {
      {
        std::shared_lock<dco_mutex_shared> lock(rwmutex);
        printf("r lock 3\n");
        dco_sleep(200);
        printf("r unlock 3\n");
      }
      // dco_sleep(1);
    }
  };

  // example main loop
  dco_epoll_run();
  return 0;
}
