/**
 * \file vm/driver_interface.hpp
 **/
#ifndef OTHERLIB_VM_DRIVER_INTERFACE_HPP
#define OTHERLIB_VM_DRIVER_INTERFACE_HPP

#include <string_view>

#include "core/defines.hpp"

namespace other {

  class driver;

  class driver_interface {
   public:
    static void set_scene_by_id(driver* driver_instance, natural_t scene_id);
  };

}  // namespace other

#endif  // OTHERLIB_VM_DRIVER_INTERFACE_HPP