/**
 * \file thread/thread_safety.cpp
 **/
#include "thread/thread_safety.hpp"

#include <thread>

namespace other {

  static bool thread_check_disabled = false;
  static std::thread::id main_thread_id;

  void register_main_thread() {
    main_thread_id = std::this_thread::get_id();
  }

  void disable_thread_check() {
    thread_check_disabled = true;
  }

  bool is_on_main_thread() {
    if (thread_check_disabled) {
      return true;
    }
    return std::this_thread::get_id() == main_thread_id;
  }

}  // namespace other