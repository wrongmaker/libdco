#include "libdco/all.h"
#include "libdco/co/coawaitable.hpp"
#include <memory>
#include <vector>

class chan_await : public dco_object<int> {
private:
  int r_;
  std::vector<chan_await *> &vec_;

public:
  chan_await(std::vector<chan_await *> &vec) : r_(0), vec_(vec) {}

public:
  virtual bool await_ready() { return false; }
  virtual void await_suspend() {
    // 这里已经挂起自己了
    vec_.push_back(this);
  }
  virtual int await_resume() { return r_; }

public:
  void set_resumt(int t) {
    r_ = t;
    resume();
  }
};

int main() {
  dco_init_worker(2);

  // 设置日志等级为debug
  spdlog::set_level(spdlog::level::debug);

  // e.g 1
  std::vector<chan_await *> vec;
  dco_chan<bool> quit;

  dco_go[&] {
    int a = 0;
    for (bool stop = false; !stop; ++a) {
      bool q;
      dco_select
          .wait_case(quit, q,
                     [&](int c) {
                       // 收到退出了
                       stop = true;
                     })
          .timeout_case(100, [](int) {
            // 等待超时
          });
      if (!vec.empty()) {
        auto await = vec.back();
        vec.pop_back();
        await->set_resumt(a);
        dco_info("await:{} set:{}", (void*)await, a);
      }
    }
    dco_info("select out");
  };

  dco_go[&] {
    for (int i = 0; i < 10; ++i) {
      chan_await wait(vec);
      int r = dco_await wait;
      dco_info("wait result:{}", r);
      dco_sleep(1000);
    }
    quit.close();
    dco_info("await out");
  };

  // example main loop
  dco_epoll_run();
}
