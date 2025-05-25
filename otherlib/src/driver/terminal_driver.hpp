/**
 * \file driver/terminal_driver.hpp
 **/
#ifndef OTHER_DRIVER_TERMINAL_DRIVER_HPP
#define OTHER_DRIVER_TERMINAL_DRIVER_HPP

#include "driver/driver.hpp"

namespace other {

  class OTHER_CLASS terminal_driver : public driver {
   public:
    terminal_driver(const config_table& config)
        : driver(config) {}
    virtual ~terminal_driver() = default;

    void initialize() override;
    void run() override;
    void shutdown() override;
  };

  driver* create_terminal_driver(const config_table& config);
  void destroy_terminal_driver(driver* instance);

}  // namespace other

#endif  // OTHER_DRIVER_TERMINAL_DRIVER_HPP