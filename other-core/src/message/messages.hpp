/**
 * \file message/messages.hpp
 **/
#ifndef OTHER_CORE_MESSAGE_MESSAGES_HPP
#define OTHER_CORE_MESSAGE_MESSAGES_HPP

#include "core/defines.hpp"

#include "message/message_defines.hpp"
#include "message/message_serialization.hpp"

namespace other {

#pragma pack(push, 1)
  struct message_header {
    uint16_t category;
    uint16_t id;
    constexpr auto operator<=>(const message_header& other) const = default;
  };
  static_assert(sizeof(message_header) == sizeof(uint32_t), "Invalid message_header size");

  struct binding_point {
    uint16_t port = 0;
    union {
      uint32_t ip;
      uint8_t bytes[4] = { 0, 0, 0, 0 };
    };

    constexpr binding_point() = default;
    constexpr binding_point(uint32_t ip, uint16_t port) : port(port), ip(ip) {}

    static std::string write_string(const binding_point& bp);
    static std::string write_string(const asio::ip::tcp::endpoint& ep);
    static std::string write_string(const asio::ip::udp::endpoint& ep);
    static binding_point from_asio(const asio::ip::address& addr, uint16_t port);
  };
  static_assert(sizeof(binding_point) == sizeof(uint32_t) + sizeof(uint16_t), "Invalid binding_point size");

  struct version {
    uint16_t major;
    uint16_t minor;
    uint16_t patch;

    std::string to_string() const {
      return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    }
  };
  static_assert(sizeof(version) == sizeof(uint16_t) * 3, "Invalid version size");

  /// ack messages
  struct acknowledgement_ack {
    natural_t ack_id;
    message_header acked_header;
    uint8_t ack = 0;  // 0 for failure, 1 for success
  };

  /// notification messages
  struct notification_connect_connection {
    binding_point connection_endpoint;
    binding_point endpoint;
    natural_t connection_id;
    natural_t new_connection_id;
    natural_t transport_hash;
  };

  struct notification_close_connection {
    natural_t connection_id;
    natural_t transport_hash;
  };

  struct notification_rx_data {
    natural_t connection_id;
    std::vector<uint8_t> data;
  };

  /// control messages
  /// command messages
  struct command_listen_connection {
    binding_point endpoint;
    natural_t connection_id;
    natural_t transport_hash;
  };

  struct command_connect_connection {
    binding_point endpoint;
    natural_t connection_id;
    natural_t transport_hash;
  };

  struct command_close_connection {
    natural_t connection_id;
    natural_t transport_hash;
  };

  struct command_tx_data {
    natural_t connection_id;
    std::vector<uint8_t> data;
  };

  struct command_attach_transport_listener {
    natural_t sink_id;
    natural_t transport_hash;
  };

  struct command_detach_transport_listener {
    natural_t sink_id;
    natural_t transport_hash;
  };

  struct command_attach_connection_sink {
    natural_t sink_id;
    natural_t connection_id;
  };

  struct command_detach_connection_sink {
    natural_t sink_id;
    natural_t connection_id;
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
  other::message_header,
  OTHER_MSG_FIELD(category, MESSAGE_CAT),
  OTHER_MSG_FIELD(id, MESSAGE_ID)
)

OTHER_REFLECT(
  other::binding_point,
  OTHER_MSG_FIELD(port, PORT_NUMBER),
  OTHER_MSG_FIELD(ip, IP_ADDRESS)
)

OTHER_REFLECT(
  other::version,
  OTHER_MSG_FIELD(major, MAJOR_VERSION_NUM),
  OTHER_MSG_FIELD(minor, MINOR_VERSION_NUM),
  OTHER_MSG_FIELD(patch, PATCH_VERSION_NUM)
)

OTHER_REFLECT(
  other::acknowledgement_ack,
  OTHER_MSG_FIELD(ack_id, ACK_ID),
  OTHER_MSG_FIELD(acked_header, ACKED_HEADER),
  OTHER_MSG_FIELD(ack, ACK)
)

OTHER_REFLECT(
  other::notification_connect_connection,
  OTHER_MSG_FIELD(connection_endpoint, CONNECTION_ENDPOINT),
  OTHER_MSG_FIELD(endpoint, ENDPOINT),
  OTHER_MSG_FIELD(connection_id, CONNECTION_ID),
  OTHER_MSG_FIELD(new_connection_id, NEW_CONNECTION_ID),
  OTHER_MSG_FIELD(transport_hash, TRANSPORT_HASH)
)

OTHER_REFLECT(
  other::notification_close_connection,
  OTHER_MSG_FIELD(connection_id, CONNECTION_ID),
  OTHER_MSG_FIELD(transport_hash, TRANSPORT_HASH)
)

OTHER_REFLECT(
  other::notification_rx_data,
  OTHER_MSG_FIELD(connection_id, CONNECTION_ID),
  OTHER_MSG_FIELD(data, DATA)
)

OTHER_REFLECT(
  other::command_listen_connection,
  OTHER_MSG_FIELD(endpoint, ENDPOINT),
  OTHER_MSG_FIELD(connection_id, CONNECTION_ID),
  OTHER_MSG_FIELD(transport_hash, TRANSPORT_HASH)
)

OTHER_REFLECT(
  other::command_connect_connection,
  OTHER_MSG_FIELD(endpoint, ENDPOINT),
  OTHER_MSG_FIELD(connection_id, CONNECTION_ID),
  OTHER_MSG_FIELD(transport_hash, TRANSPORT_HASH)
)

OTHER_REFLECT(
  other::command_close_connection,
  OTHER_MSG_FIELD(connection_id, CONNECTION_ID),
  OTHER_MSG_FIELD(transport_hash, TRANSPORT_HASH)
)

OTHER_REFLECT(
  other::command_tx_data,
  OTHER_MSG_FIELD(connection_id, CONNECTION_ID),
  OTHER_MSG_FIELD(data, DATA)
)

OTHER_REFLECT(
  other::command_attach_transport_listener,
  OTHER_MSG_FIELD(sink_id, SINK_ID),
  OTHER_MSG_FIELD(transport_hash, TRANSPORT_HASH)
)

OTHER_REFLECT(
  other::command_detach_transport_listener,
  OTHER_MSG_FIELD(sink_id, SINK_ID),
  OTHER_MSG_FIELD(transport_hash, TRANSPORT_HASH)
)

OTHER_REFLECT(
  other::request_acknowledgment,
  OTHER_MSG_FIELD(ack_id, ACK_ID),
  OTHER_MSG_FIELD(original_header, ACKED_HEADER),
  OTHER_MSG_FIELD(message_data, DATA)
)

namespace std {

  template <>
  struct formatter<other::message_header> : public formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const other::message_header& header, FormatContext& ctx) const {
      const std::string fmt = std::format("[{}.{}]", other::message_category{ header.category }, other::message_id{ header.id });
      return formatter<std::string_view>::format(fmt, ctx);
    }
  };

  template <>
  struct formatter<other::binding_point> : public formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const other::binding_point& bp, FormatContext& ctx) const {
      const std::string fmt = std::format("{}.{}.{}.{}:{}", bp.bytes[0], bp.bytes[1], bp.bytes[2], bp.bytes[3], bp.port);
      return formatter<std::string_view>::format(fmt, ctx);
    }
  };

}  // namespace std

#endif  // OTHER_CORE_MESSAGE_MESSAGES_HPP