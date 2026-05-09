/**
 * \file thread/thread_safety.hpp
 **/
#ifndef OTHER_CORE_THREAD_THREAD_SAFETY_HPP
#define OTHER_CORE_THREAD_THREAD_SAFETY_HPP

#include "core/defines.hpp"

namespace other {

  void register_main_thread();
  bool is_on_main_thread();

}  // namespace other

#define ASSERT_MAIN_THREAD() \
  OTHER_ASSERT(other::is_on_main_thread(), "This function must be called from the main thread.")

#endif  // OTHER_CORE_THREAD_THREAD_SAFETY_HPP