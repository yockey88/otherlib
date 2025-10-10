/**
 * \file ${project-name}_driver.hpp
 **/
#ifndef OTHER_${PROJECT_NAME_UPPER}_DRIVER_HPP
#define OTHER_${PROJECT_NAME_UPPER}_DRIVER_HPP

#include "other.hpp"

namespace ${project-name} {

 class OTHER_CLASS ${project-name}_driver {
  public:
   ${project-name}_driver() = default;
   ~${project-name}_driver() = default;

    void on_initialize(const command_line& cmd) override;
    void run() override;
    void on_shutdown() override;
 };

} // namespace ${project-name}

OTHER_DRIVER(${project-name}::${project-name}_driver);

#endif // OTHER_${PROJECT_NAME_UPPER}_DRIVER_HPP