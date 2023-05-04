#include "libdco/hook/coerrno.h"
#include <atomic>
#include <stdio.h>
#include <thread>

extern int *__errno_location(void);
extern "C" volatile int *libdco__errno_location(void) {
  std::atomic_thread_fence(std::memory_order_seq_cst);
  // printf("hook errno in libdco.\n");
  return (volatile int *)__errno_location();
}