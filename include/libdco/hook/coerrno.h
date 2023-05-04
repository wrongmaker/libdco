#pragma once
#include <errno.h>

#ifdef errno
extern "C" volatile int *libdco__errno_location(void) __attribute__((noinline));
#undef errno
#define errno (*libdco__errno_location())
#endif
