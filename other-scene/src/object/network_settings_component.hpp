/**
 * \file object/network_settings_component.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_NETWORK_SETTINGS_COMPONENT_HPP
#define OTHER_SCENE_OBJECT_NETWORK_SETTINGS_COMPONENT_HPP

#include <string>

#include "core/defines.hpp"
#include "serialization/reflection.hpp"

namespace other {

  /// authored session policy (Mode 1): first-found settings object wins; replicas ignore it (the
  ///  session you joined is the session); consulted by the session layer at playback start — scene stays networking-free
  struct network_settings_component {
    std::string session_mode = "off";  // "off" | "host" | "join"
    std::string transport = "";        // "" = networking.transport | "tcp" | "steam"
    std::string address = "";          // join target "ip:port"; empty + steam = invite-driven
    uint16_t port = 0;                 // 0 = networking.port
    uint16_t max_peers = 0;            // 0 = networking.max-peers
    /// name of a disabled template object; empty = no auto-spawn on peer join
    std::string spawn_template = "";
  };

}  // namespace other

OTHER_REFLECT(
  other::network_settings_component,
  field(session_mode, other::attr::serializable("Session Mode")),
  field(transport, other::attr::serializable("Transport")),
  field(address, other::attr::serializable("Address")),
  field(port, other::attr::serializable("Port")),
  field(max_peers, other::attr::serializable("Max Peers")),
  field(spawn_template, other::attr::serializable("Spawn Template")))

#endif  // OTHER_SCENE_OBJECT_NETWORK_SETTINGS_COMPONENT_HPP
