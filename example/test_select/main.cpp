#include "libdco/all.h"

int main() {
  // dco_init();

  // // 设置日志等级为debug
  // spdlog::set_level(spdlog::level::debug);

  // // e.g 1
  // dco_chan<int, 0> chan;
  // dco_go[&] {
  //   int i = 0;
  //   for (;;) {
  //     chan.post(i);
  //     dco_debug("e.g 1 post:{}", i);
  //     i += 1;
  //     if (i == 10) {
  //       dco_debug("e.g 1 finish");
  //       chan.close();
  //       break;
  //     }
  //     dco_sleep(100);
  //   }
  // };

  // dco_go[&] {
  //   for (bool stop = false; !stop;) {
  //     int out = 0;
  //     dco_select.wait_case(chan, out, [&](int code) {
  //       if (code == -2) {
  //         stop = true;
  //         return;
  //       }
  //       dco_debug("e.g 1 wait :{}", out);
  //     });
  //   }
  //   dco_debug("e.g 1 select out");
  // };

  // // e.g 2
  // dco_chan<int, 0> product;
  // dco_chan<bool, 0> quit;
  // dco_go[&] {
  //   for (int i = 0; i < 10; ++i) {
  //     int out = 0;
  //     int ret = product.wait(out);
  //     dco_debug("e.g 2 get:{} ret:{}", out, ret);
  //     dco_sleep(1000);
  //   }
  //   quit.post(true);
  //   quit.close();
  // };

  // dco_go[&] {
  //   int put = 0;
  //   for (bool stop = false; !stop;) {
  //     bool out = 0;
  //     dco_select
  //         .post_case(product, put,
  //                    [&](int code) {
  //                      put += 1;
  //                      dco_debug("e.g 2 post:{}", put);
  //                    })
  //         .wait_case(quit, out, [&](int code) {
  //           dco_debug("e.g 2 quit");
  //           product.close();
  //           stop = true;
  //         });
  //   }
  //   dco_debug("e.g 2 select out");
  // };

  // dco_chan<int, 0> chan1;
  // // e.g 3
  // dco_go[&] {
  //   int out = 0;
  //   dco_select.wait_case(chan1, out, [](int) {})
  //       .timeout_case(1000, [](int code) { dco_debug("timeout"); });
  // };

  // // e.g 3
  // dco_go[&] {
  //   int out = 0;
  //   dco_select.wait_case(chan1, out, [](int) {}).default_case([]() {
  //     dco_debug("default");
  //   });
  // };

  // // example main loop
  // dco_epoll_run();
}
