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
    CORE_LOG_INFO("Creating Python interpreter...");
    guard = arena_allocator<py::scoped_interpreter>{}.allocate();
  }

  void python_interpreter::unload_host() {
    PROFILE_SECTION("python_interpreter::unload-host");
    CORE_LOG_INFO("Destroying Python interpreter...");
    arena_allocator<py::scoped_interpreter>{}.free(guard);
    guard = nullptr;
  }

  void python_interpreter::call_entry_point() {
    PROFILE_SECTION("python_interpreter::call-entry-point");
    if (guard) {
    }
  }

}  // namespace other