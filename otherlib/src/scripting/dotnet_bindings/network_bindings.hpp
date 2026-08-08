/**
 * \file scripting/dotnet_bindings/network_bindings.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_DOTNET_BINDINGS_NETWORK_BINDINGS_HPP
#define OTHERLIB_SCRIPTING_DOTNET_BINDINGS_NETWORK_BINDINGS_HPP

#include "dotnet/native_string.hpp"
#include "dotnet/types.hpp"

namespace other {
  namespace bindings {

    nbool32 native_network_is_connected();
    int32_t native_network_get_role();
    int32_t native_network_local_peer_id();

    nbool32 native_network_host(uint16_t port);
    nbool32 native_network_join(native_string address, uint16_t port);
    void native_network_leave();

    void native_network_send_event(native_string event_name, const uint8_t* payload, int32_t length);
    void native_network_broadcast_event(native_string event_name, const uint8_t* payload, int32_t length);
    /// pulls the payload parked by the in-flight DispatchEvent call
    int32_t native_network_copy_event_payload(uint8_t* dst, int32_t capacity);

  }  // namespace bindings
}  // namespace other

#endif  // OTHERLIB_SCRIPTING_DOTNET_BINDINGS_NETWORK_BINDINGS_HPP
