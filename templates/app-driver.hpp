/**
 * \file ${project-name}_driver.hpp
 **/
#ifndef OTHER_${project-name-upper}_DRIVER_HPP
#define OTHER_${project-name-upper}_DRIVER_HPP

#include "other.hpp"

namespace ${project-name} {

 class OTHER_CLASS ${project-name}_driver : public other::driver {
  public:
   ${project-name}_driver(const other::config_table& config) 
      : driver(config) {}
   ~${project-name}_driver() override = default;

    void on_initialize(const command_line& cmd) override;
    void run() override;
    void on_shutdown() override;
 };

} // namespace ${project-name}

OTHER_DRIVER(${project-name}::${project-name}_driver);

#endif // OTHER_${project-name-upper}_DRIVER_HPP