/**
 * \file python/interpreter.cpp
 **/
#include "python/interpreter.hpp"

#include <pybind11/embed.h>

#include "core/arena_allocator.hpp"
#include "core/logger.hpp"

namespace py = pybind11;

namespace other {

  void python_interpreter::load_host() {
    CORE_LOG_DEBUG("Creating Python interpreter...");
    guard = arena_allocator<py::scoped_interpreter>{}.allocate();
  }

  void python_interpreter::unload_host() {
    CORE_LOG_DEBUG("Destroying Python interpreter...");
    arena_allocator<py::scoped_interpreter>{}.free(guard);
    guard = nullptr;
  }

  void python_interpreter::call_entry_point() {
    if (guard) {
    }
  }

}  // namespace other