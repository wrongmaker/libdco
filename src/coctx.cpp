#include "libdco/co/coctx.h"

using namespace dco;

void coctx::default_pull(coroutine<void>::push_type &) {}

void coctx::default_push(coroutine<void>::pull_type &) {}

const char *coctx::get_status_name(coctx_status_full status) {
  switch (status) {
  case Ready_Reset_Reset:
    return "Ready_Reset_Reset";
  case Yield_Reset_Reset:
    return "Yield_Reset_Reset";
  case Yield_Set_Reset:
    return "Yield_Set_Reset";
  case Yield_Break_Reset:
    return "Yield_Break_Reset";
  case Sleep_Reset_Reset:
    return "Sleep_Reset_Reset";
  case Sleep_Set_Reset:
    return "Sleep_Set_Reset";
  case Sleep_Break_Reset:
    return "Sleep_Break_Reset";
  case Sleep_Set_Set:
    return "Sleep_Set_Set";
  case Sleep_Set_Break:
    return "Sleep_Set_Break";
  case Sleep_Set_Remove:
    return "Sleep_Set_Remove";
  }
  return "unknown";
}

#define COCTX_STATUS_NEW(current, current_status)                              \
  (((((current) >> VERSION_SHIFT) + 1) << VERSION_SHIFT) | (current_status))

#define COCTX_STATUS_RESET ((0 << VERSION_SHIFT) | (Yield_Set_Reset))

#define COCTX_STATUS_GET(current) (((current) & STATUS_MASK) >> STATUS_SHIFT)

coctx::coctx_status_full coctx::get_status() {
  return (coctx_status_full)(status_.load() & STATUS_MASK);
}

int coctx::try_resume_full() {
  int res;
  uint64_t current = status_.load(), new_status;
  do {
    // 重置结果
    res = -1;
    // 分解当前状态和版本号
    uint8_t current_status = (uint8_t)(current & STATUS_MASK);
    // 尝试进行状态切换
    if (current_status == Yield_Set_Reset) {
      current_status = Ready_Reset_Reset;
      res = 0; // 此处直接加入队列等待
    } else if (current_status == Yield_Reset_Reset) {
      current_status = Yield_Break_Reset;
      res = 1; // 此处需要调度唤醒
    } else if (current_status == Sleep_Reset_Reset) {
      current_status = Sleep_Break_Reset;
      res = 2; // 下次add直接唤醒
    } else if (current_status == Sleep_Set_Reset) {
      current_status = Sleep_Set_Break;
      res = 3; // 等待下一步操作
    } else if (current_status == Sleep_Set_Set) {
      current_status = Sleep_Set_Remove;
      res = 4; // 加入移除队列中去
    } else {
      // printf("try_resume_full ctx id:%d status:%s\n", id_,
      //   get_status_name((coctx_status_full)current_status));
      break;
    }
    // 生成新的状态码
    new_status = COCTX_STATUS_NEW(current, current_status);
    // 尝试进行变更
  } while (!status_.compare_exchange_strong(current, new_status));
  return res;
}

int coctx::try_yield_full() {
  int res;
  uint64_t current = status_.load(), new_status;
  do {
    // 重置结果
    res = -1;
    // 分解当前状态和版本号
    uint8_t current_status = (uint8_t)(current & STATUS_MASK);
    // 尝试进行状态切换
    if (current_status == Ready_Reset_Reset) {
      current_status = Yield_Reset_Reset;
      res = 1; // 等待下一步
    } else {
      // printf("try_yield_full ctx id:%d status:%s\n", id_,
      //        get_status_name((coctx_status_full)current_status));
      break;
    }
    // 生成新的状态码
    new_status = COCTX_STATUS_NEW(current, current_status);
    // 尝试进行变更
  } while (!status_.compare_exchange_strong(current, new_status));
  BOOST_ASSERT(res != -1);
  return res;
}

int coctx::try_sw_full() {
  int res;
  uint64_t current = status_.load(), new_status;
  do {
    // 重置结果
    res = -1;
    // 分解当前状态和版本号
    uint8_t current_status = (uint8_t)(current & STATUS_MASK);
    // 尝试进行状态切换
    if (current_status == Yield_Reset_Reset) {
      current_status = Yield_Set_Reset;
      res = 1; // 等待下一步操作
    } else if (current_status == Yield_Break_Reset) {
      current_status = Ready_Reset_Reset;
      res = 0; // 此处需要调度唤醒
    } else if (current_status == Sleep_Reset_Reset) {
      current_status = Sleep_Set_Reset;
      res = 2; // 需要加入定时器等待队列
    } else if (current_status == Sleep_Break_Reset) {
      current_status = Ready_Reset_Reset;
      res = 0; // 此处需要调度唤醒
    } else {
      // printf("try_sw_full ctx id:%d status:%s\n", id_,
      //        get_status_name((coctx_status_full)current_status));
      break;
    }
    // 生成新的状态码
    new_status = COCTX_STATUS_NEW(current, current_status);
    // 尝试进行变更
  } while (!status_.compare_exchange_strong(current, new_status));
  BOOST_ASSERT(res != -1);
  return res;
}

int coctx::try_sleep_full() {
  int res;
  uint64_t current = status_.load(), new_status;
  do {
    // 重置结果
    res = -1;
    // 分解当前状态和版本号
    uint8_t current_status = (uint8_t)(current & STATUS_MASK);
    // 尝试进行状态切换
    if (current_status == Ready_Reset_Reset) {
      current_status = Sleep_Reset_Reset;
      res = 1; // 等待下一步
    } else {
      // printf("try_sleep_full ctx id:%d status:%s in_que:%d\n", id_,
      //        get_status_name((coctx_status_full)current_status),
      //        in_que_.load());
      break;
    }
    // 生成新的状态码
    new_status = COCTX_STATUS_NEW(current, current_status);
    // 尝试进行变更
  } while (!status_.compare_exchange_strong(current, new_status));
  BOOST_ASSERT(res != -1);
  return res;
}

int coctx::try_sleep_add_full() {
  int res;
  uint64_t current = status_.load(), new_status;
  do {
    // 重置结果
    res = -1;
    // 分解当前状态和版本号
    uint8_t current_status = (uint8_t)(current & STATUS_MASK);
    // 尝试进行状态切换
    if (current_status == Sleep_Set_Reset) {
      current_status = Sleep_Set_Set;
      res = 1; // 加入定时器等待超时
    } else if (current_status == Sleep_Set_Break) {
      current_status = Ready_Reset_Reset;
      res = 0; // 未加入定时器被打断唤醒
    } else {
      // printf("try_sleep_add_full ctx id:%d status:%s\n", id_,
      //        get_status_name((coctx_status_full)current_status));
      break;
    }
    // 生成新的状态码
    new_status = COCTX_STATUS_NEW(current, current_status);
    // 尝试进行变更
  } while (!status_.compare_exchange_strong(current, new_status));
  BOOST_ASSERT(res != -1);
  return res;
}

int coctx::try_sleep_remove_full() {
  int res;
  uint64_t current = status_.load(), new_status;
  do {
    // 重置结果
    res = -1;
    // 分解当前状态和版本号
    uint8_t current_status = (uint8_t)(current & STATUS_MASK);
    // 尝试进行状态切换
    if (current_status == Sleep_Set_Remove) {
      current_status = Ready_Reset_Reset;
      res = 0; // 加入定时器后被打断唤醒
    } else {
      // printf("try_sleep_remove_full ctx id:%d status:%s\n", id_,
      //        get_status_name((coctx_status_full)current_status));
      break;
    }
    // 生成新的状态码
    new_status = COCTX_STATUS_NEW(current, current_status);
    // 尝试进行变更
  } while (!status_.compare_exchange_strong(current, new_status));
  return res;
}

int coctx::try_sleep_timeout_full() {
  int res;
  uint64_t current = status_.load(), new_status;
  do {
    // 重置结果
    res = -1;
    // 分解当前状态和版本号
    uint8_t current_status = (uint8_t)(current & STATUS_MASK);
    // 尝试进行状态切换
    if (current_status == Sleep_Set_Set) {
      current_status = Ready_Reset_Reset;
      res = 0; // 被超时唤醒
    } else {
      // printf("try_sleep_timeout_full ctx id:%d status:%s\n", id_,
      //        get_status_name((coctx_status_full)current_status));
      break;
    }
    // 生成新的状态码
    new_status = COCTX_STATUS_NEW(current, current_status);
    // 尝试进行变更
  } while (!status_.compare_exchange_strong(current, new_status));
  return res;
}

bool coctx::in_sleep() { return COCTX_STATUS_GET(status_) == 0b10; }

bool coctx::in_yield() { return COCTX_STATUS_GET(status_) == 0b01; }

void coctx::set_timeout(volatile time_t timeout) {
  BOOST_ASSERT(index_ == (size_t)-1);
  timeout_ = timeout;
  break_ = true;
  index_ = (size_t)-1;
}

void coctx::reset() {
  status_.store(COCTX_STATUS_RESET);
  // run_ = false;
  end_ = false;
  set_timeout(0);
  swout_ =
      std::move(coroutine<void>::pull_type(segmented_stack(), default_pull));
  swin_ =
      std::move(coroutine<void>::push_type(segmented_stack(), default_push));
}

coctx::coctx(uint32_t id)
    : swout_(std::move(
          coroutine<void>::pull_type(segmented_stack(), default_pull))),
      swin_(std::move(
          coroutine<void>::push_type(segmented_stack(), default_push))),
      status_(COCTX_STATUS_RESET), index_((size_t)-1), timeout_(0), id_(id),
      break_(false), end_(false) {}