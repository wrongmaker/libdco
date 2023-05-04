#include "libdco/all.h"

int main() {
  dco_init();

  // 设置日志等级为debug
  spdlog::set_level(spdlog::level::debug);

  dco_chan<std::shared_ptr<std::string>> chan_test;

  dco_chan<int> chan;
  dco_go[&] {
    int i = 0;
    for (;;) {
      // chan.post(i);
      chan << i;
      dco_info("post:{}", i);
      i += 1;
      if (i == 10) {
        dco_info("finish");
        chan.close();
        break;
      }
    }
  };
  dco_go[&] {
    for (;;) {
      int out;
      dco_chan<int>::ec_cochannel ev(out);
      chan >> ev;
      if (ev.ec_ == -2) {
        dco_info("chan close");
        break;
      }
      dco_info("wait:{}", out);
      dco_sleep(1000);
    }
  };

  dco_chan<int> chan_buff(5);
  dco_go[&] {
    int i = 0;
    for (;;) {
      chan_buff.post(i);
      dco_info("buff post:{}", i);
      i += 1;
      if (i == 10) {
        dco_info("post finish");
        chan_buff.close();
        break;
      }
    }
  };
  dco_go[&] {
    for (;;) {
      int out = 0;
      int ec = chan_buff.wait(out);
      if (ec == -2) {
        dco_info("buff chan close");
        break;
      }
      dco_info("buff wait:{}", out);
      dco_sleep(1000);
    }
  };

  // example main loop
  dco_epoll_run();
  return 0;
}

// #include "libdco/all.h"

// int main() {
//   dco_init();

//   // 设置日志等级为debug
//   spdlog::set_level(spdlog::level::debug);

//   auto start = std::chrono::high_resolution_clock::now();
//   auto fini = start;

//   const int kDef = 100000;
//   const int kCount = 2;
//   int res = kDef * kCount;

//   dco_chan<int> chan;
//   dco_go[&] {
//     start = std::chrono::high_resolution_clock::now();
//     for (int i = 0; i < kDef; ++i) {
//       chan << i;
//     }
//     chan.close();
//   };
//   dco_go[&] {
//     int out;
//     for (;;) {
//       dco_chan<int>::ec_cochannel ev(out);
//       chan >> ev;
//       if (ev.ec_ == -2) {
//         break;
//       }
//     }
//     fini = std::chrono::high_resolution_clock::now();
//     auto timecost = fini - start;
//     dco_debug("time cost:{} mean:{} out:{}", timecost.count(),
//               timecost.count() / (double)res, out);
//   };

//   // example main loop
//   dco_epoll_run();
//   return 0;
// }
