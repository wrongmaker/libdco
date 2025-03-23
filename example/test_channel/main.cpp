#include "libdco/all.h"
#include "libdco/co/coctx.h"

int main() {
  dco_init_worker(1);

  int times = 1000000;

  dco_chan<int, 0> chan_buff;
  dco_go[&](dco::coctx * ctx) {
    int i = 0;
    for (;;) {
      time_t start = time(0);
      int res = chan_buff.post(i);
      if (res != 1) {
        // printf("cid:%d 2 buff post:%d ec:%d time:%ld\n", ctx->id(), i, res,
        //        time(0) - start);
        continue;
      }
      i += 1;
      if (i == times) {
        dco_sleep(1000);
        printf("cid:%d 2 buff post finish i:%d\n", ctx->id(), i);
        chan_buff.close();
        break;
      }
      // dco_sleep(500);
    }
  };

  // dco_go[&](dco::coctx * ctx) {
  //   int i = times;
  //   for (;;) {
  //     time_t start = time(0);
  //     int res = chan_buff.post(i);
  //     if (res == -2) {
  //       printf("cid:%d 3 buff close i:%d\n", ctx->id(), i);
  //       break;
  //     }
  //     if (res != 1) {
  //       // printf("cid:%d 3 buff post:%d ec:%d time:%ld\n", ctx->id(), i, res,
  //       //        time(0) - start);
  //       continue;
  //     }
  //     // dco_sleep(1);
  //     i += 1;
  //     if (i == times + times) {
  //       printf("cid:%d 3 buff post finish i:%d\n", ctx->id(), i - times);
  //       break;
  //     }
  //     // dco_sleep(500);
  //   }
  // };

  dco_chan<int, 2> chan_res;

  time_t start = time(0);
  dco_go[&](dco::coctx * ctx) {
    int times = 0;
    for (;;) {
      int out = 0;
      int ec = chan_buff.wait(out);
      // int ec = chan_buff.wait( out);
      if (ec == -2) {
        chan_res.post(times);
        printf("cid:%d 1 wait finish get:%d cost:%ld\n", ctx->id(), times,
               time(0) - start);
        break;
      }
      // printf("cid:%d buff wait:%d\n", ctx->id(), out);
      if (ec == 1)
        ++times;
      // else
      //   printf("cid:%d 1 buff wait:%d ec:%d\n", ctx->id(), out, ec);
    }
  };

  // dco_go[&](dco::coctx * ctx) {
  //   int times = 0;
  //   for (;;) {
  //     int out = 0;
  //     int ec = chan_buff.wait(out);
  //     // int ec = chan_buff.wait( out);
  //     if (ec == -2) {
  //       chan_res.post(times);
  //       printf("cid:%d 2 wait finish get:%d cost:%ld\n", ctx->id(), times,
  //              time(0) - start);
  //       break;
  //     }
  //     // printf("cid:%d buff wait:%d\n", ctx->id(), out);
  //     if (ec == 1)
  //       ++times;
  //     // else
  //     //   printf("cid:%d 2 buff wait:%d ec:%d\n", ctx->id(), out, ec);
  //   }
  // };

  dco_go[&] {
    int times = 0;
    for (;;) {
      int out;
      if (chan_res.wait(out) == 1) {
        times += out;
        printf("add times:%d\n", times);
        dco_sche->dco_show_worker_info();
      }
    }
  };

  // example main loop
  dco_epoll_run();
  return 0;
}
