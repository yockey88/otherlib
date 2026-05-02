/**
 * \file core/subsystem.cpp
 **/
#include "core/subsystem.hpp"

#include "core/logger.hpp"

namespace other {

  void unactive_subsystem_initialization_error(const std::string_view subsystem_name) {
    throw std::runtime_error(std::format("Subsystem {} is inert and cannot be initialized without an instance being set.\nAttempting to grab subsystem at:\n{}", subsystem_name, OTHER_STACKTRACE));
  }

}  // namespace other