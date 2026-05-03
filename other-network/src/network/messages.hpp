/**
 * \file network/messages.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_MESSAGES_HPP
#define OTHER_NETWORK_NETWORK_MESSAGES_HPP

#include "core/defines.hpp"
#include "thread/message.hpp"

#include "network/message.hpp"

namespace other {

#pragma pack(push, 1)
  struct acknowledgement {
    natural_t ack_id;
    message_header acked_header;
    uint8_t ack = 0;  // 0 for failure, 1 for success
  };
  static_assert(is_readable_field<message_header>, "message_header must be a readable field to be used in acknowledgement");
  static_assert(is_writable_field<message_header>, "message_header must be a writable field to be used in acknowledgement");

  struct listen_tcp_connection_request {
    binding_point endpoint;
    natural_t connection_id;
  };
  static_assert(is_readable_field<binding_point>, "binding_point must be a readable field to be used in listen_tcp_connection_request");
  static_assert(is_writable_field<binding_point>, "binding_point must be a writable field to be used in listen_tcp_connection_request");

#pragma pack(pop)

}  // namespace other

OTHER_REFLECT(
  other::acknowledgement,
  field(ack_id, other::attr::serializable("ack-id")),
  field(acked_header, other::attr::serializable("acked-header")),
  field(ack, other::attr::serializable("ack"))
)

OTHER_REFLECT(
  other::listen_tcp_connection_request,
  field(endpoint, other::attr::serializable("endpoint")),
  field(connection_id, other::attr::serializable("connection-id"))
)

#endif  // OTHER_NETWORK_NETWORK_MESSAGES_HPP