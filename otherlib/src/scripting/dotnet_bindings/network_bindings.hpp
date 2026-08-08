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

    nbool32 native_network_spawn(uint64_t object_id, uint16_t owner_peer);
    /// component_key = the codec key ("physics", "render", "network", ...)
    nbool32 native_network_sync_component(uint64_t object_id, native_string component_key);
    nbool32 native_network_is_mine(uint64_t object_id);

    nbool32 native_network_request_op(native_string op_name, uint64_t subject_net_id, const uint8_t* payload, int32_t length);
    /// C# -> native blob handoff during a [Replicated] collect call
    void native_network_stage_replicated(const uint8_t* data, int32_t length);

  }  // namespace bindings
}  // namespace other

#endif  // OTHERLIB_SCRIPTING_DOTNET_BINDINGS_NETWORK_BINDINGS_HPP
