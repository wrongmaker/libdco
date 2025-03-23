#include "libdco/co/comutexshared.h"

using namespace dco;

static int _rw_max_count = 1 << 30;

comutexshared::comutexshared(coschedule *sche)
    : count_(0), wait_(0), rsem_(sche), wsem_(sche), wmtx_(sche) {}

comutexshared::~comutexshared() {}

// 如果 (count_ -= _rw_max_count) + _rw_max_count; -> --count_ -> --wait_ ->
// (wait_ += r) 因为 --wait_ < 0 != 0 所以 wait_ += r 可能 = 0 所以就不挂起写
// 即使wsem 还未wait 但notify了，也能够被唤醒

bool comutexshared::lock() {
  // 1. 先进行锁定
  wmtx_.lock();
  // 2. 标记有写锁，获取锁读时读数量
  int r = (count_ -= _rw_max_count) + _rw_max_count;
  // 3. 等待读操作完成，并等待在读释放
  if (r != 0 && (wait_ += r) != 0)
    return wsem_.wait();
  return true;
}

bool comutexshared::lock_shared() {
  // 1. 有写锁在持有或等待
  if ((count_ += 1) < 0)
    return rsem_.wait();
  return true;
}

void comutexshared::unlock() {
  // 1. 恢复count的值
  int r = (count_ += _rw_max_count);
  // 2. 唤醒所有的读锁
  while (r--)
    rsem_.notify();
  // 3. 释放锁
  wmtx_.unlock();
}

void comutexshared::unlock_shared() {
  // 1. 释放读锁
  if (--count_ >= 0)
    return;
  // 2. 存在写等待，若为最后的读则唤醒写
  if (--wait_ == 0)
    wsem_.notify();
}
