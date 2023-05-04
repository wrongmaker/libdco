#include "libdco/hook/cofdcollect.h"

#include <memory>
#include <mutex>

#include "libdco/co/coctx.h"
#include "libdco/co/coschedule.h"
#include "libdco/hook/cofdctx.h"

using namespace dco;

cofdcollect::cofdcollect(coschedule *sche) : ptr_sche_(sche) {}

cofdcollect::~cofdcollect() {}

cofdcollect::cofdpack_ptr cofdcollect::get_insert(int fd, bool insert) {
  int bucketid = fd & cofdcollect::kBucketMask;
  auto &bucket = buckets_[bucketid];
  if (insert) {
    // 需要插入的 打写锁
    std::unique_lock<std::shared_timed_mutex> lock(bucket.mtx_);
    auto iter = bucket.packs_.find(fd);
    if (iter != bucket.packs_.end())
      return iter->second;
    cofdpack_ptr ptr = std::make_shared<cofdpack>();
    bucket.packs_[fd] = ptr;
    return ptr;
  }
  // 获取的 打读锁
  std::shared_lock<std::shared_timed_mutex> lock(bucket.mtx_);
  auto iter = bucket.packs_.find(fd);
  if (iter != bucket.packs_.end())
    return iter->second;
  return nullptr;
}

cofdctx_ptr cofdcollect::get(int fd) {
  auto fd_ptr = get_insert(fd);
  if (fd_ptr == nullptr)
    return nullptr;
  // 加个锁防止修改
  std::lock_guard<boost::detail::spinlock> lock(fd_ptr->mtx_);
  return fd_ptr->fdctx_;
}

void cofdcollect::del(int fd) {
  // printf("cofdcollect del fd:%d\n", fd);
  auto fd_ptr = get_insert(fd);
  if (fd_ptr == nullptr || fd_ptr->fdctx_ == nullptr)
    return;
  // 获取老的
  cofdctx_ptr one = nullptr;
  // 更新ctx
  {
    std::lock_guard<boost::detail::spinlock> lock(fd_ptr->mtx_);
    one.swap(fd_ptr->fdctx_);
    // printf("swap %d\n", fd_ptr->fdctx_ == nullptr);
  }
  // 假设有东西
  if (one) {
    one->close();
  }
}

void cofdcollect::create(int fd, bool block) {
  auto fd_ptr = get_insert(fd, true);
  // 获取老的
  cofdctx_ptr one = std::make_shared<cofdctx>();
  // 更新ctx
  {
    std::lock_guard<boost::detail::spinlock> lock(fd_ptr->mtx_);
    one.swap(fd_ptr->fdctx_);
    // 设置信息
    fd_ptr->fdctx_->fd_set(fd);
    fd_ptr->fdctx_->isblock_set(block);
  }
  dco_fd_debug("co create fd:{}", fd);
  // 假设有东西
  if (one) {
    one->close();
    dco_fd_debug("dco close old fd:{}", fd_ptr->fdctx_->fd_);
  }
}

coschedule *cofdcollect::sche_ptr() const { return ptr_sche_; }
