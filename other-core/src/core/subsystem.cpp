/**
 * \file core/subsystem.cpp
 **/
#include "core/subsystem.hpp"

#include "core/logger.hpp"

namespace other {

  void unactive_subsystem_initialization_error(const std::string_view subsystem_name) {
    const std::string msg = std::format(
      "Subsystem {} is inert and cannot be initialized without an instance being set.\n"
      "(If this fires before main: a static-storage object is using an arena-backed "
      "container during static initialization.)\nAttempting to grab subsystem at:\n{}",
      subsystem_name, OTHER_STACKTRACE);
    std::fputs(msg.c_str(), stderr);
    throw std::runtime_error(msg);
  }

}  // namespace other