#include "libdco/all.h"
#include <chrono>

int main() {
  dco_init_worker(1);

  // getchar();

  // 设置日志等级为debug
  spdlog::set_level(spdlog::level::debug);

  auto start = std::chrono::high_resolution_clock::now();
  auto fini = start;

  const int kDef = 1000000;
  const int kCount = 4;
  int res = kDef * kCount;
  std::atomic<int> nt{0};

  dco_go[&res, &start, &nt, &fini] {
    printf("start\n");
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < kDef; ++i) {
      //++res;
      dco_gosche; // 让出cpu
      // dco_debug("sche 1\n");
    }
    ++nt;
    if (nt == kCount) {
      fini = std::chrono::high_resolution_clock::now();
      auto timecost = fini - start;
      dco_debug("time cost:{} mean:{}", timecost.count(),
                timecost.count() / (double)res);
      exit(0);
    }
    printf("finish 0 res:%d\n", res);
  };

  dco_go[&res, &fini, &nt, &start] {
    for (int i = 0; i < kDef; ++i) {
      //++res;
      dco_gosche; // 让出cpu
      // dco_debug("sche 1\n");
    }
    ++nt;
    if (nt == kCount) {
      fini = std::chrono::high_resolution_clock::now();
      auto timecost = fini - start;
      dco_debug("time cost:{} mean:{}", timecost.count(),
                timecost.count() / (double)res);
      exit(0);
    }
    printf("finish 1 res:%d\n", res);
  };

  dco_go[&res, &fini, &nt, &start] {
    for (int i = 0; i < kDef; ++i) {
      //++res;
      dco_gosche; // 让出cpu
      // dco_debug("sche 1\n");
    }
    ++nt;
    if (nt == kCount) {
      fini = std::chrono::high_resolution_clock::now();
      auto timecost = fini - start;
      dco_debug("time cost:{} mean:{}", timecost.count(),
                timecost.count() / (double)res);
      exit(0);
    }
    printf("finish 2 res:%d\n", res);
  };

  dco_go[&res, &fini, &nt, &start] {
    for (int i = 0; i < kDef; ++i) {
      //++res;
      dco_gosche; // 让出cpu
      // dco_debug("sche 2\n");
    }
    ++nt;
    if (nt == kCount) {
      fini = std::chrono::high_resolution_clock::now();
      auto timecost = fini - start;
      dco_debug("time cost:{} mean:{}", timecost.count(),
                timecost.count() / (double)res);
      exit(0);
    }
    printf("finish 3 res:%d\n", res);
  };

  // printf("res:%d\n", res);
  // 大概切换时间为0.081微秒

  // example main loop
  dco_epoll_run();
  return 0;
}
