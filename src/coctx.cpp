#include "libdco/co/coctx.h"

#include <iostream>

using namespace dco;

void coctx::default_pull(coroutine<void>::push_type &) {}

void coctx::default_push(coroutine<void>::pull_type &) {}

void coctx::set_resume() {
  sw_ = coctx::Reset;
  add_ = coctx::Reset;
  status_ = coctx::Ready;
}

int coctx::try_resume() {
#if DCO_MULT_THREAD
#if DCO_CONTEXT_SPINLOCK
  std::lock_guard<boost::detail::spinlock> lock(mtx_);
#else
  std::lock_guard<std::mutex> lock(mtx_);
#endif
#endif
  if (status_ == coctx::Ready) {
    return -1;
  } else if (status_ == coctx::Yield) {
    if (sw_ == coctx::Set) {
      set_resume();
      return 0;
    } else if (sw_ == coctx::Reset) {
      sw_ = coctx::Break;
      return 2;
    }
    return -2;
  } else if (status_ == coctx::Sleep) {
    if (sw_ == coctx::Set) {
      if (add_ == coctx::Set) {
        // set_resume();
        status_ = coctx::SleepBreak;
        return 103;
      } else if (add_ == coctx::Reset) {
        add_ = coctx::Break;
        return 3;
      }
    } else if (sw_ == coctx::Reset) {
      if (add_ == coctx::Reset) {
        sw_ = coctx::Break;
        add_ = coctx::Break;
        return 3;
      } else if (add_ == coctx::Set) {
        sw_ = coctx::Break;
        return 3;
      }
    }
    return -3;
  } else if (status_ == coctx::SleepBreak) {
    return -4;
  }
  return -99;
}

int coctx::try_yield() {
#if DCO_MULT_THREAD
#if DCO_CONTEXT_SPINLOCK
  std::lock_guard<boost::detail::spinlock> lock(mtx_);
#else
  std::lock_guard<std::mutex> lock(mtx_);
#endif
#endif
  if (status_ == coctx::Ready) {
    if (sw_ == coctx::Reset) {
      status_ = coctx::Yield;
      return 0;
    }
    return -1;
  } else if (status_ == coctx::Yield) {
    return -2;
  } else if (status_ == coctx::Sleep) {
    return -3;
  } else if (status_ == coctx::SleepBreak) {
    return -4;
  }
  return -99;
}

int coctx::try_sleep() {
#if DCO_MULT_THREAD
#if DCO_CONTEXT_SPINLOCK
  std::lock_guard<boost::detail::spinlock> lock(mtx_);
#else
  std::lock_guard<std::mutex> lock(mtx_);
#endif
#endif
  if (status_ == coctx::Ready) {
    if (sw_ == coctx::Reset && add_ == coctx::Reset) {
      status_ = coctx::Sleep;
      return 0;
    }
    return -1;
  } else if (status_ == coctx::Yield) {
    return -2;
  } else if (status_ == coctx::Sleep) {
    return -3;
  } else if (status_ == coctx::SleepBreak) {
    return -4;
  }
  return -99;
}

int coctx::try_sleepbreak() {
#if DCO_MULT_THREAD
#if DCO_CONTEXT_SPINLOCK
  std::lock_guard<boost::detail::spinlock> lock(mtx_);
#else
  std::lock_guard<std::mutex> lock(mtx_);
#endif
#endif
  if (status_ == coctx::Ready) {
    return -1;
  } else if (status_ == coctx::Yield) {
    return -2;
  } else if (status_ == coctx::Sleep) {
    return -3;
  } else if (status_ == coctx::SleepBreak) {
    set_resume();
    return 0;
  }
  return -99;
}

int coctx::try_sw() {
#if DCO_MULT_THREAD
#if DCO_CONTEXT_SPINLOCK
  std::lock_guard<boost::detail::spinlock> lock(mtx_);
#else
  std::lock_guard<std::mutex> lock(mtx_);
#endif
#endif
  if (status_ == coctx::Ready) {
    return -1;
  } else if (status_ == coctx::Yield) {
    if (sw_ == coctx::Reset) {
      sw_ = coctx::Set;
      return 2;
    } else if (sw_ == coctx::Break) {
      set_resume();
      return 0;
    }
    return -2;
  } else if (status_ == coctx::Sleep) {
    if (sw_ == coctx::Reset) {
      if (add_ == coctx::Set) {
        sw_ = coctx::Set;
        return 3;
      } else if (add_ == coctx::Reset) {
        sw_ = coctx::Set;
        return 3;
      }
    } else if (sw_ == coctx::Break) {
      if (add_ == coctx::Set) {
        // set_resume();
        status_ = coctx::SleepBreak;
        return 103; // 这里是因为加入堆后未挂起被打断了
      } else if (add_ == coctx::Break) {
        sw_ = coctx::Set;
        return 3;
      }
    }
    return -3;
  } else if (status_ == coctx::SleepBreak) {
    return -4;
  }
  return -99;
}

int coctx::try_add() {
#if DCO_MULT_THREAD
#if DCO_CONTEXT_SPINLOCK
  std::lock_guard<boost::detail::spinlock> lock(mtx_);
#else
  std::lock_guard<std::mutex> lock(mtx_);
#endif
#endif
  if (status_ == coctx::Ready) {
    return -1;
  } else if (status_ == coctx::Yield) {
    return -2;
  } else if (status_ == coctx::Sleep) {
    if (sw_ == coctx::Set) {
      if (add_ == coctx::Reset) {
        add_ = coctx::Set;
        return 3;
      } else if (add_ == coctx::Break) {
        set_resume();
        return 0;
      }
    } else if (sw_ == coctx::Reset) {
      if (add_ == coctx::Reset) {
        add_ = coctx::Set;
        return 3;
      }
    } else if (sw_ == coctx::Break) {
      if (add_ == coctx::Break) {
        add_ = coctx::Set;
        return 3;
      }
    }
    return -3;
  } else if (status_ == coctx::SleepBreak) {
    return -4;
  }
  return -99;
}

int coctx::try_reset_resume() {
#if DCO_MULT_THREAD
#if DCO_CONTEXT_SPINLOCK
  std::lock_guard<boost::detail::spinlock> lock(mtx_);
#else
  std::lock_guard<std::mutex> lock(mtx_);
#endif
#endif
  if (status_ == coctx::Ready) {
    return -1;
  } else if (status_ == coctx::Yield) {
    if (sw_ == coctx::Reset) {
      set_resume();
      return 0;
    }
    return -2;
  } else if (status_ == coctx::Sleep) {
    if (sw_ == coctx::Reset && add_ == coctx::Reset) {
      set_resume();
      return 0;
    }
    return -3;
  } else if (status_ == coctx::SleepBreak) {
    return -4;
  }
  return -99;
}

int coctx::try_gosche() {
#if DCO_MULT_THREAD
#if DCO_CONTEXT_SPINLOCK
  std::lock_guard<boost::detail::spinlock> lock(mtx_);
#else
  std::lock_guard<std::mutex> lock(mtx_);
#endif
#endif
  if (status_ == coctx::Ready) {
    if (sw_ == coctx::Reset) {
      status_ = coctx::Yield;
      sw_ = coctx::Break;
      return 0;
    }
    return -1;
  } else if (status_ == coctx::Yield) {
    return -2;
  } else if (status_ == coctx::Sleep) {
    return -3;
  } else if (status_ == coctx::SleepBreak) {
    return -4;
  }
  return -99;
}

void coctx::set_timeout(volatile time_t timeout) {
  BOOST_ASSERT(index_ == (size_t)-1);
  timeout_ = timeout;
  break_ = false;
  index_ = (size_t)-1;
}

void coctx::reset() {
  status_ = coctx::Yield;
  sw_ = coctx::Set;
  add_ = coctx::Reset;
  run_ = false;
  end_ = false;
  set_timeout(0);
  pull_ = std::move(coroutine<void>::pull_type(default_pull));
  push_ = std::move(coroutine<void>::push_type(default_push));
}

coctx::coctx(uint32_t id)
    : pull_(std::move(coroutine<void>::pull_type(default_pull))),
      push_(std::move(coroutine<void>::push_type(default_push))),
      status_(coctx::Yield), sw_(coctx::Set), add_(coctx::Reset),
#if DCO_MULT_THREAD
#if DCO_CONTEXT_SPINLOCK
      mtx_ BOOST_DETAIL_SPINLOCK_INIT,
#endif
#endif
      index_((size_t)-1), timeout_(0), id_(id), break_(false), run_(false),
      end_(false)

{
}