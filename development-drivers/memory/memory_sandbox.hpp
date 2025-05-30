/**
 * \file memory/memory_sandbox.hpp
 **/
#ifndef OTHER_MEMORY_SANDBOX_HPP
#define OTHER_MEMORY_SANDBOX_HPP

#include <glm/glm.hpp>

#include "core/defines.hpp"
#include "driver/driver.hpp"

class OTHER_CLASS memory_sandbox : public other::driver {
 public:
  memory_sandbox(const other::config_table& config)
      : other::driver(config) {}

  void on_initialize() override;
  void run() override;
  void on_shutdown() override;
};

OTHER_DRIVER(memory_sandbox)

#endif  // OTHER_MEMORY_SANDBOX_HPP