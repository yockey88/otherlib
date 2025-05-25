/**
 * \file driver/terminal_driver.cpp
 **/
#include "driver/terminal_driver.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#include "core/logger.hpp"

namespace other {

  void terminal_driver::initialize() {
  }

  void terminal_driver::run() {
  }

  void terminal_driver::shutdown() {
  }

  driver* create_terminal_driver(const config_table& config) {
    return new terminal_driver(config);
  }

  void destroy_terminal_driver(driver* instance) {
    delete instance;
  }

}  // namespace other