/**
 * \file network/driver_messages.hpp
 **/
#ifndef OTHER_LIB_NETWORK_DRIVER_MESSAGES_HPP
#define OTHER_LIB_NETWORK_DRIVER_MESSAGES_HPP

#include "core/defines.hpp"
#include "thread/message.hpp"

#include "network/message.hpp"

namespace other {

#pragma pack(push, 1)
  struct listen_tcp_connection_request {
    binding_point endpoint;
    natural_t connection_id;
  };
#pragma pack(pop)

}  // namespace other

OTHER_REFLECT(
  other::listen_tcp_connection_request,
  field(endpoint, other::attr::message_field("endpoint")),
  field(connection_id, other::attr::message_field("connection_id"))
)

#endif  // OTHER_LIB_NETWORK_DRIVER_MESSAGES_HPP