/**
 * \file scripting/interfaces/connection_handler_interface.cpp
 **/
#include "scripting/interfaces/connection_handler_interface.hpp"

namespace other {

  environment_interface get_connection_handler_script_interface() {
    environment_interface env = {
      .name = "connection_handler",
      .actions = {
        action("OnAcceptConnection", "Called when a new connection is accepted."),
        action("OnEstablishConnection", "Called when a new connection is established."),
      }
    };
    return env;
  }

  environment_interface get_connection_handler_plugin_interface() {
    environment_interface env = {
      .name = "connection_handler_plugin",
      .actions = {
        action("on_accept_connection", "Called when a new connection is accepted."),
        action("on_establish_connection", "Called when a new connection is established."),
      }
    };
    return env;
  }

}  // namespace other