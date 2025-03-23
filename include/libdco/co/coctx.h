#pragma once
#include "boost/coroutine2/all.hpp"
#include <cstdint>

namespace dco {
using namespace boost::coroutines2;

template <class T> class cominheap;
template <class T> class cominheap_mt;
class coschedule;
class coworker;
class coctx {
  static constexpr uint64_t STATUS_MASK = 0xFF;                // 低8位：状态
  static constexpr uint64_t VERSION_MASK = 0xFFFFFFFFFFFFFF00; // 高56位：版本号
  static constexpr int VERSION_SHIFT = 8;
  static constexpr int STATUS_SHIFT = 4;

private:
  // [status] ready,yield
  // [sw] reset,set,break
  // [sleep] reset,set,break,remove

  // 状态
  enum coctx_status_full : uint8_t {
    // [status][sw][sleep]
    // reset
    Ready_Reset_Reset = 0b00'00'00,
    // yield
    Yield_Reset_Reset = 0b01'00'00,
    Yield_Set_Reset = 0b01'01'00,
    Yield_Break_Reset = 0b01'10'00,
    // sleep
    Sleep_Reset_Reset = 0b10'00'00,
    Sleep_Set_Reset = 0b10'01'00,
    Sleep_Break_Reset = 0b10'10'00,
    Sleep_Set_Set = 0b10'01'01,
    Sleep_Set_Break = 0b10'01'10,
    Sleep_Set_Remove = 0b10'01'11,
  };

  // 状态迁移
  // Ready_Reset_Reset -yield-> Yield_Reset_Reset
  // Yield_Reset_Reset -sw-> Yield_Set_Reset
  // Yield_Set_Reset -resume-> Ready_Reset_Reset
  // Yield_Reset_Reset -resume-> Yield_Break_Reset
  // Yield_Break_Reset -sw-> Ready_Reset_Reset
  // Ready_Reset_Reset -sleep-> Sleep_Reset_Reset
  // Sleep_Reset_Reset -resume-> Sleep_Break_Reset
  // Sleep_Break_Reset -sw-> Ready_Reset_Reset
  // Sleep_Reset_Reset -sw-> Sleep_Set_Reset
  // Sleep_Set_Reset -resume-> Sleep_Set_Break
  // Sleep_Set_Break -add-> Ready_Reset_Reset
  // Sleep_Set_Reset -add-> Sleep_Set_Set
  // Sleep_Set_Set -timeout-> Ready_Reset_Reset
  // Sleep_Set_Set -resume-> Sleep_Set_Remove
  // Sleep_Set_Remove -remove-> Ready_Reset_Reset

private:
  coroutine<void>::pull_type swout_;
  coroutine<void>::push_type swin_;
  // 状态码
  std::atomic<uint64_t> status_;
  // 协程id
  uint32_t id_;
  // 线程id
  volatile uint32_t tid_;
  // 超时相关
  volatile size_t index_;
  volatile time_t timeout_;
  volatile bool break_;
  // 运行结束
  volatile bool end_;

private:
  friend class coschedule;
  friend class coworker;
  friend class cominheap<coctx>;
  friend class cominheap_mt<coctx>;

private:
  const char *get_status_name(coctx_status_full status);
  coctx_status_full get_status();
  static void default_pull(coroutine<void>::push_type &);
  static void default_push(coroutine<void>::pull_type &);

private:
  int try_resume_full();
  int try_yield_full();
  int try_sw_full();
  int try_sleep_full();
  int try_sleep_add_full();
  int try_sleep_remove_full();
  int try_sleep_timeout_full();

private:
  void set_timeout(volatile time_t timeout);
  void reset();

public:
  bool in_sleep();
  bool in_yield();
  uint32_t id() const { return id_; }

public:
  bool operator<(const coctx &obj) const { return timeout_ < obj.timeout_; }

public:
  coctx(uint32_t id);
  coctx(const coctx &) = delete;
};

} // namespace dco
