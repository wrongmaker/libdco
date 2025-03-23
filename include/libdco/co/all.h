#pragma once
#include "libdco/co/coawaitable.hpp"
#include "libdco/co/cochannel.hpp"
#include "libdco/co/cocondvariable.h"
#include "libdco/co/coctx.h"
#include "libdco/co/cofutex.h"
#include "libdco/co/cogenerator.hpp"
#include "libdco/co/comutex.h"
#include "libdco/co/comutexshared.h"
#include "libdco/co/coschedule.h"
#include "libdco/co/coselect.h"
#include "libdco/co/cosemaphore.h"
#include "libdco/util/conolist.hpp"
#include "libdco/util/cosingleton.hpp"
#include <functional>

#define __dco_sche dco::cosingleton<dco::coschedule>::instance()
#define __dco_sche_ptr __dco_sche.get()

namespace dco {
struct dco_cogo {
  void operator=(const std::function<void()> &func) {
    auto ctx = __dco_sche->dco_create(func);
    __dco_sche->dco_resume(ctx);
  }

  void operator=(const std::function<void(coctx *)> &func) {
    auto ctx = __dco_sche->dco_create(func);
    __dco_sche->dco_resume(ctx);
  }
};

class dco_comutex : public comutex {
public:
  dco_comutex() : comutex(__dco_sche_ptr) {}
};

class dco_cocond : public cocondvariable {
public:
  dco_cocond() : cocondvariable(__dco_sche_ptr) {}
};

template <class T, int N> class dco_cochan : public cochannel<T, N> {
public:
  dco_cochan() : cochannel<T, N>(__dco_sche_ptr) {}
};

class dco_comutex_shared : public comutexshared {
public:
  dco_comutex_shared() : comutexshared(__dco_sche_ptr) {}
};

template <class Main_, class Func_>
class dco_cogen : public cogenerator<Main_, Func_> {
public:
  dco_cogen() : cogenerator<Main_, Func_>(__dco_sche_ptr) {}

public:
  void operator=(
      const std::function<void(cogenerator_yield<Main_, Func_> &)> &func) {
    cogenerator<Main_, Func_>::create(func);
  }
};

class dco_cosem : public cosemaphore {
public:
  dco_cosem(int def = 0) : cosemaphore(__dco_sche_ptr, def) {}
};

class dco_cosem_tm : public cosemaphore_tm {
public:
  dco_cosem_tm(int def = 0) : cosemaphore_tm(__dco_sche_ptr, def) {}
};

template <class T> class dco_coobject : public coobject<T> {
public:
  dco_coobject() : coobject<T>(__dco_sche_ptr) {}
  virtual ~dco_coobject() {}
};

template <> class dco_coobject<void> : public coobject<void> {
public:
  dco_coobject() : coobject<void>(__dco_sche_ptr) {}
  virtual ~dco_coobject() {}
};

// template <class T, int N> struct dco_case_wait {
//   dco_cochan<T, N> *chan_;
//   T &t_;
//   dco::coselect &select_;
//   dco_case_wait(dco_cochan<T, N> *chan, T &t, dco::coselect &select)
//       : chan_(chan), t_(t), select_(select) {}
//   dco_case_wait(dco_cochan<T, N> &chan, T &t, dco::coselect &select)
//       : chan_(std::addressof(chan)), t_(t), select_(select) {}

//   // recv wait
//   void operator-(const std::function<void(int)> &func) {
//     select_.wait_case(chan_, t_, func);
//   }
// };

// template <class T, int N> struct dco_case_post {
//   dco_cochan<T, N> *chan_;
//   T &t_;
//   dco::coselect &select_;
//   dco_case_post(dco_cochan<T, N> *chan, T &t, dco::coselect &select)
//       : chan_(chan), t_(t), select_(select) {}
//   dco_case_post(dco_cochan<T, N> &chan, T &t, dco::coselect &select)
//       : chan_(std::addressof(chan)), t_(t), select_(select) {}

//   // send post
//   void operator-(const std::function<void(int)> &func) {
//     select_.post_case(chan_, t_, func);
//   }
// };

// struct dco_case_timeout {
//   time_t ms_;
//   dco::coselect &select_;

//   dco_case_timeout(time_t ms, dco::coselect &select)
//       : ms_(ms), select_(select) {}
//   // timeout
//   void operator-(const std::function<void(int)> &func) {
//     select_.timeout_case(ms_, func);
//   }
// };

// struct dco_case_default {
//   dco::coselect &select_;
//   dco_case_default(dco::coselect &select) : select_(select) {}
//   // default
//   void operator-(const std::function<void()> &func) {
//     select_.default_case(func);
//   }
// };

struct dco_coyield {
  void operator=(const std::function<void(coctx *)> &func) {
    auto ctx = __dco_sche->dco_curr_ctx();
    __dco_sche->dco_yield(ctx, func);
  }
};

} // namespace dco
