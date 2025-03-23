#include "libdco/all.h"
#include <chrono>

int main() {
  dco_init_worker(4);

  // dco_sem sem;
  dco_sem_tm sem;
  int i0 = 0, i1 = 0;
  dco_go[&sem, &i0, &i1] {
    dco_sleep(100);
    int time = 0;
    int try_times = 25;
    while (try_times--) {
      time = 1000;
      for (; time--;) {
        sem.notify();
        // dco_sleep(1);
      }
      printf("s i0:%d i1:%d %d\n", i0, i1, i0 + i1);
      dco_sleep(1000);
      printf("s i0:%d i1:%d %d\n", i0, i1, i0 + i1);
    }
  };

  dco_go[&sem, &i0, &i1] {
    dco_sleep(100);
    int time = 0;
    int try_times = 25;
    while (try_times--) {
      time = 1000;
      for (; time--;) {
        sem.notify();
        // dco_sleep(1);
      }
      printf("e i0:%d i1:%d %d\n", i0, i1, i0 + i1);
      dco_sleep(1000);
      printf("e i0:%d i1:%d %d\n", i0, i1, i0 + i1);
    }
  };

  dco_go[&sem, &i0](dco::coctx * ctx) {
    printf("0 %p\n", ctx);
    for (i0 = 0;; i0++) {
      // sem.wait_for(100);
      if (sem.wait_for(100) != 1)
        --i0;
      // } else {
      //   // printf("0 timeout\n");
      // }
      // printf("0 %d\n", i0);
    }
  };

  dco_go[&sem, &i1](dco::coctx * ctx) {
    printf("1 %p\n", ctx);
    for (i1 = 0;; i1++) {
      // sem.wait_for(100);
      if (sem.wait_for(100) != 1)
        --i1;
      // } else {
      //   // printf("1 timeout\n");
      // }
      // printf("1 %d\n", i1);
    }
  };

  // printf("res:%d\n", res);
  // 大概切换时间为0.081微秒

  // example main loop
  dco_epoll_run();
  return 0;
}
