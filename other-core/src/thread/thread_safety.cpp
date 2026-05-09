/**
 * \file thread/thread_safety.cpp
 **/
#include "thread/thread_safety.hpp"

#include <thread>

namespace other {

  static std::thread::id main_thread_id;

  void register_main_thread() {
    main_thread_id = std::this_thread::get_id();
  }

  bool is_on_main_thread() {
    return std::this_thread::get_id() == main_thread_id;
  }

}  // namespace other