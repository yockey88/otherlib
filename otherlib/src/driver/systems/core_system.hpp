/**
 * \file driver/systems/core_system.hpp
 **/
#ifndef OTHERLIB_DRIVER_SYSTEMS_CORE_SYSTEM_HPP
#define OTHERLIB_DRIVER_SYSTEMS_CORE_SYSTEM_HPP

#include "driver/driver_kernel.hpp"
#include "driver/driver_system.hpp"

namespace other {

  class driver;

  /// CRTP base class for core driver systems
  template <typename D>
  class core_system : public driver_system {
   public:
    core_system(driver* driver_instance, uint32_t id)
        : driver_system(driver_instance, id) {}

   protected:
    template <typename T>
    T& sibling(driver_kernel& kernel) {
      return kernel.get_core_system<T>();
    }

    template <typename T>
    const T& sibling(const driver_kernel& kernel) const {
      return kernel.get_core_system<T>();
    }

    template <typename T>
    bool has_sibling(const driver_kernel& kernel) const {
      return kernel.has_core_system<T>();
    }
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_SYSTEMS_CORE_SYSTEM_HPP