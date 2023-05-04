#pragma once
#include <iostream>
#include <memory>

namespace dco {

template <class T> class cosingleton {
private:
  static std::shared_ptr<T> instance_;

public:
  // 线程不安全
  static void init(std::shared_ptr<T> ptr) { instance_ = ptr; }

  // static c++11后线程安全
  static std::shared_ptr<T> instance() { return instance_; }

public:
  cosingleton() = delete;
  cosingleton(const cosingleton &) = delete;
  ~cosingleton() {}
};

template <class T> std::shared_ptr<T> cosingleton<T>::instance_ = nullptr;

} // namespace dco
