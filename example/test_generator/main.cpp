#include "libdco/all.h"

void generator_test(dco_gen_yield<int, int> &yield) {
  for (int i = 0; i < 10; ++i) {
    yield(i);
  }
}

void generator_test2(dco_gen_yield<int, int> &yield) {
  int i = 0;
  int j = yield(i++); // yield
  dco_info("e.g 3 j:{}", j);
  j = yield(i++);
  dco_info("e.g 3 j:{}", j);
  j = yield(i++);
  dco_info("e.g 3 j:{}", j);
  j = yield(i++);
  dco_info("e.g 3 j:{}", j);
  j = yield(i++);
  dco_info("e.g 3 j:{}", j);
}

int main() {
  dco_init();

  // 设置日志等级为debug
  spdlog::set_level(spdlog::level::debug);

  dco_go[] {
    dco_gen<int, int> gen;
    gen = generator_test;
    for (auto i : gen) {
      dco_info("i:{}", i);
      dco_sleep(100);
    }
  };

  dco_go[] {
    dco_gen<int, int> gen;
    gen = [](dco_gen_yield<int, int> &yield) {
      for (int i = 0; i < 10; ++i) {
        yield(i);
      }
    };
    for (auto i : gen) {
      dco_info("i:{}", i);
      dco_sleep(1000);
    }
  };

  dco_go[] {
    dco_gen<int, int> gen;
    gen = generator_test2;
    int i = gen(10);
    dco_info("e.g 3 i:{}", i);
    i = gen(11);
    dco_info("e.g 3 i:{}", i);
    i = gen(12);
    dco_info("e.g 3 i:{}", i);
    i = gen(13);
    dco_info("e.g 3 i:{}", i);
    gen(14); // call finish
  };

  // example main loop
  dco_epoll_run();
  return 0;
}
