#pragma once
#include <iostream>

#include "cocasque.hpp"
#ifdef _WIN32
#include <windows.h>
#else
#include <semaphore.h>
#endif

namespace dco {

class semaphore {
private:
#ifdef _WIN32
  HANDLE sem_;
#else
  sem_t sem_;
  int max_;
#endif

public:
  semaphore(int max) {
#ifdef _WIN32
    sem_ = CreateSemaphore(NULL, 0, max, NULL);
#else
    sem_init(&sem_, 0, 0);
    max_ = max;
#endif
  }
  ~semaphore() {
#ifdef _WIN32
    CloseHandle(sem_);
#else
    sem_destroy(&sem_);
#endif
  }

public:
  void try_wait() {
#ifdef _WIN32
    WaitForSingleObject(sem_, INFINITE);
#else
    sem_wait(&sem_);
#endif
  }

  void try_wait_for(time_t ms) {
#ifdef _WIN32
    WaitForSingleObject(sem_, (DWORD)ms);
#else
    time_t rs = ms / 1000;
    time_t rms = ms % 1000;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    rms = rms * 1000000 + ts.tv_nsec;
    rs = rms / 1000000000 + rs;
    rms = rms % 1000000000;
    ts.tv_sec += rs;
    ts.tv_nsec = rms;
    // printf("ms:%ld rs:%ld rms:%ld\n", ms, rs, rms);
    sem_timedwait(&sem_, &ts);
#endif
  }

  void try_notify() {
#ifdef _WIN32
    ReleaseSemaphore(sem_, 1, NULL);
#else
    int sval = 0;
    sem_getvalue(&sem_, &sval);
    // printf("sem_:%d\n", sval);
    if (sval < max_)
      sem_post(&sem_);
#endif
  }
};

} // namespace dco
