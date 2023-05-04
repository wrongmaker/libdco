#pragma once

#define DCO_MULT_THREAD 1  // 多线程支持

#define DCO_CONTEXT_SPINLOCK 1  // 协程上下文状态是否使用原子自旋锁

#if (DCO_MULT_THREAD == 0)
#define DCO_SINGLE_LOOP_BLOCK 1  // 单线程主循环是否阻塞
#endif

#define DCO_DISABLE_WRITE_LOG 0
