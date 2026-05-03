/**
 * \file scripting/interfaces/connetion_handler_interface.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_INTERFACES_CONNECTION_HANDLER_INTERFACE_HPP
#define OTHERLIB_SCRIPTING_INTERFACES_CONNECTION_HANDLER_INTERFACE_HPP

#include "scripting/environment_interface.hpp"

namespace other {

  environment_interface get_connection_handler_script_interface();
  environment_interface get_connection_handler_plugin_interface();

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_INTERFACES_CONNECTION_HANDLER_INTERFACE_HPP