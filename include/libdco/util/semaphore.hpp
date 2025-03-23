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
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    // 将毫秒转换为纳秒并累加到当前时间
    uint64_t timeout_ns = (uint64_t)ms * 1000000ULL;
    uint64_t total_ns = (uint64_t)ts.tv_nsec + timeout_ns;

    // 处理进位并更新timespec
    ts.tv_sec += total_ns / 1000000000ULL;
    ts.tv_nsec = total_ns % 1000000000ULL;

    // 执行等待
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
