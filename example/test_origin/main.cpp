#include "libdco/all.h"
#include <chrono>

int main() {
  dco_init_worker(1);

  auto start = std::chrono::high_resolution_clock::now();
  auto fini = start;

  const int kDef = 10000000;
  const int kCount = 10;
  int res = kDef * kCount;
  std::atomic<int> nt{0};

  printf("start\n");
  for (int i = 0; i < kCount; ++i) {
    dco_go[&res, &start, &nt, &fini, i] {
      start = std::chrono::high_resolution_clock::now();
      for (int i = 0; i < kDef; ++i) {
        dco_swout; // 让出cpu
      }
      ++nt;
      if (nt == kCount) {
        fini = std::chrono::high_resolution_clock::now();
        auto timecost = fini - start;
        dco_debug("time cost:{} mean:{}", timecost.count(),
                  timecost.count() / (double)res);
        exit(0);
      }
      printf("finish %d res:%d\n", i, res);
    };
  }
  
  // example main loop
  dco_epoll_run();
  return 0;
}
