/**
 * \file driver/terminal_driver.cpp
 **/
#include "driver/terminal_driver.hpp"

namespace other {

  void terminal_driver::on_initialize() {
  }

  void terminal_driver::on_run() {
  }

  void terminal_driver::on_shutdown() {
  }

  driver* create_terminal_driver(const config_table& config) {
    return new terminal_driver(config);
  }

  void destroy_terminal_driver(driver* instance) {
    delete instance;
  }

}  // namespace other