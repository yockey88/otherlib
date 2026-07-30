/**
 * \file python/interpreter.cpp
 **/
#include "python/interpreter.hpp"

#include <pybind11/embed.h>

#include "core/logger.hpp"
#include "core/profiler.hpp"
#include "memory/arena_allocator.hpp"

namespace py = pybind11;

namespace other {

  void python_interpreter::load_host() {
    PROFILE_SECTION("python_interpreter::load-host");
  }

  void python_interpreter::unload_host() {
    PROFILE_SECTION("python_interpreter::unload-host");
  }

  void python_interpreter::call_entry_point() {
    PROFILE_SECTION("python_interpreter::call-entry-point");
  }

}  // namespace other