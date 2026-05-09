/**
 * \file scripting/interfaces/networking_interfaces.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_INTERFACES_NETWORKING_INTERFACES_HPP
#define OTHERLIB_SCRIPTING_INTERFACES_NETWORKING_INTERFACES_HPP

#include "scripting/environment_interface.hpp"

namespace other {

  environment_interface get_server_interface();
  environment_interface get_http_server_interface();

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_INTERFACES_NETWORKING_INTERFACES_HPP