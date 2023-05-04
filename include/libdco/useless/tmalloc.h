#pragma once
#include <stddef.h>
#include <stdint.h>

// 这是个简单实现 按这个实现会出现许多内存碎片
// 使用jemalloc来代替

// 在glibc中，内存分配相关函数都是弱符号，jemalloc直接替换命名了。。。
// 详见jemalloc/jemalloc.h 中 #define je_xx xx 直接替换了相关函数
// 就是这个步骤替换的malloc,free等。。。
// 还有一种就是用glibc malloc hook的方法的
// 使用__attribute__((alias (#je_fn), used))改宏进行方法替代

namespace dco {

// class tmalloc {
//  public:
//   static void* talloc(size_t);
//   static void tfree(void*);
// };

// class tbuffer {
//  public:
//   tbuffer() {}
//   virtual ~tbuffer() {}

//  public:
//   void* operator new(size_t s);
//   void operator delete(void* p);
// };

}  // namespace dco