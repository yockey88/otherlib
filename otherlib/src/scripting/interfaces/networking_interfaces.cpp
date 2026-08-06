/**
 * \file scripting/interfaces/networking_interfaces.cpp
 **/
#include "scripting/interfaces/networking_interfaces.hpp"

namespace other {

  environment_interface get_server_interface() {
    return {
      .name = "Other.Server",
      .description = "Interface to listen on a TCP port for connections and handle data received by those connections.",
      .actions = {
        {
          .name = "AcceptConnection",
          .description = "Callback invoked when a new connection is accepted. Args: (from_connection_id [uint64_t], new_connection_id [uint64_t])",
          .script_name = "OnAcceptConnection",
          .plugin_name = "on_accept_connection",
        },
        {
          .name = "ReceiveData",
          .description = "Callback invoked when data is received on a connection. Args: (connection_id [uint64_t], data [byte array])",
          .script_name = "OnReceiveData",
          .plugin_name = "on_receive_data",
        },
        {
          .name = "CloseConnection",
          .description = "Callback invoked when a connection is closed. Args: (connection_id [uint64_t])",
          .script_name = "OnCloseConnection",
          .plugin_name = "on_close_connection",
        },
      }
    };
  }

}  // namespace other