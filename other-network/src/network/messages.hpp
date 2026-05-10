/**
 * \file network/messages.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_MESSAGES_HPP
#define OTHER_NETWORK_NETWORK_MESSAGES_HPP

#include "core/defines.hpp"
#include "thread/message.hpp"

namespace other {

#pragma pack(push, 1)

  /// ack messages
  struct acknowledgement_ack {
    natural_t ack_id;
    message_header acked_header;
    uint8_t ack = 0;  // 0 for failure, 1 for success
  };

  /// notification messages
  struct notification_connect_tcp_connection {
    binding_point connection_endpoint;
    binding_point endpoint;
    natural_t connection_id;
    natural_t new_connection_id;
  };

  struct notification_close_tcp_connection {
    natural_t connection_id;
  };

  struct notification_rx_data {
    natural_t connection_id;
    std::vector<uint8_t> data;
  };

  /// control messages
  /// command messages
  struct command_listen_tcp_connection {
    binding_point endpoint;
    natural_t connection_id;
    natural_t transport_hash;
  };

  struct command_connect_tcp_connection {
    binding_point endpoint;
    natural_t connection_id;
    natural_t transport_hash;
  };

  struct command_tx_data {
    natural_t connection_id;
    std::vector<uint8_t> data;
  };

  /// request/response messages
  struct request_acknowledgment {
    natural_t ack_id;
    message_header original_header;
    std::vector<uint8_t> message_data;
  };

  /// error alert messages

#pragma pack(pop)

}  // namespace other

OTHER_REFLECT(
  other::acknowledgement_ack,
  field(ack_id, other::attr::serializable("ack-id")),
  field(acked_header, other::attr::serializable("acked-header")),
  field(ack, other::attr::serializable("ack"))
)
OTHER_REFLECT(
  other::notification_connect_tcp_connection,
  field(connection_endpoint, other::attr::serializable("connection-endpoint")),
  field(endpoint, other::attr::serializable("endpoint")),
  field(connection_id, other::attr::serializable("connection-id")),
  field(new_connection_id, other::attr::serializable("new-connection-id"))
)

OTHER_REFLECT(
  other::notification_close_tcp_connection,
  field(connection_id, other::attr::serializable("connection-id"))
)

OTHER_REFLECT(
  other::notification_rx_data,
  field(connection_id, other::attr::serializable("connection-id")),
  field(data, other::attr::serializable("data"))
)

OTHER_REFLECT(
  other::command_listen_tcp_connection,
  field(endpoint, other::attr::serializable("endpoint")),
  field(connection_id, other::attr::serializable("connection-id")),
  field(transport_hash, other::attr::serializable("transport-hash"))
)

OTHER_REFLECT(
  other::command_connect_tcp_connection,
  field(endpoint, other::attr::serializable("endpoint")),
  field(connection_id, other::attr::serializable("connection-id")),
  field(transport_hash, other::attr::serializable("transport-hash"))
)

OTHER_REFLECT(
  other::command_tx_data,
  field(connection_id, other::attr::serializable("connection-id")),
  field(data, other::attr::serializable("data"))
)

OTHER_REFLECT(
  other::request_acknowledgment,
  field(ack_id, other::attr::serializable("ack-id")),
  field(original_header, other::attr::serializable("original-header")),
  field(message_data, other::attr::serializable("message-data"))
)

#endif  // OTHER_NETWORK_NETWORK_MESSAGES_HPP