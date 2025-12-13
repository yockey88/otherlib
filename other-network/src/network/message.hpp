/**
 * \file network/message.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_MESSAGE_HPP
#define OTHER_NETWORK_NETWORK_MESSAGE_HPP

#include "thread/message.hpp"
#include "thread/test/new_message.hpp"

#include "network/udp_datagram.hpp"

namespace other {

#pragma pack(push, 1)
  struct message_header_tcp {
    message_header header;
    uint32_t packet_size = 0;
  };

  struct network_packet_header {
    uint32_t packet_size = 0;
    uint16_t packet_type = 0;
    uint16_t num_messages = 0;
  };
#pragma pack(pop)

  struct tcp_packet {
    constexpr static inline size_t kMaxSize = 1448 - sizeof(network_packet_header);

    network_packet_header net_header;
    std::vector<message> messages;
  };

#pragma pack(push, 1)

  struct udp_binding_information;

  /// notification messages
  struct notification_session_check_in : other_message_spec_impl<notification_session_check_in> {
    integer_t session_id = 0;
  };

  struct notification_session_closed : other_message_spec_impl<notification_session_closed> {
    integer_t session_id = 0;
  };

  struct notification_stream_rx_datagram : other_message_spec_impl<notification_stream_rx_datagram> {
    integer_t stream_id = 0;
    udp_datagram datagram;

    static std::vector<uint8_t> custom_builder(notification_stream_rx_datagram* msg);
    static notification_stream_rx_datagram custom_parser(const std::span<const uint8_t> data);
  };

  /// acknowledgement messages

  /// control messages
  struct control_ping : other_message_spec_impl<control_ping> {
    integer_t session_id = 0;
  };

  struct control_pong : other_message_spec_impl<control_pong> {
    integer_t session_id = 0;
  };

  /// command messages
  struct command_session_connect_to : other_message_spec_impl<command_session_connect_to> {
    binding_point address;
  };

  struct command_session_listen_at : other_message_spec_impl<command_session_listen_at> {
    binding_point address;
  };

  struct command_session_check_in_at : other_message_spec_impl<command_session_check_in_at> {
    integer_t session_id = 0;
    binding_point address;
  };

  struct command_session_tx_message : other_message_spec_impl<command_session_tx_message> {
    integer_t session_id = 0;
    message msg;

    static std::vector<uint8_t> custom_builder(command_session_tx_message* msg);
    static command_session_tx_message custom_parser(const std::span<const uint8_t> data);
  };

  struct command_load_empty_scene : other_message_spec_impl<command_load_empty_scene> {
    uint8_t session_id_flag = 0;
    integer_t session_id = 0;

    uint8_t requires_udp_binding = 0;
    binding_point udp_address;
    binding_point server_udp_address;

    std::string scene_name;

    static std::vector<uint8_t> custom_builder(command_load_empty_scene* msg);
    static command_load_empty_scene custom_parser(const std::span<const uint8_t> data);
  };

  struct command_stream_send_udp_datagram : other_message_spec_impl<command_stream_send_udp_datagram> {
    integer_t stream_id = 0;
    udp_datagram datagram;

    static std::vector<uint8_t> custom_builder(command_stream_send_udp_datagram* msg);
    static command_stream_send_udp_datagram custom_parser(const std::span<const uint8_t> data);
  };

  /// request messages
  struct session_check_in_request : other_message_spec_impl<session_check_in_request> {
    integer_t session_id = 0;
  };

  struct session_information_request : other_message_spec_impl<session_information_request> {
    uint8_t project_data_flag = 0;
    uint8_t name_flag = 0;
    uint8_t executable_flag = 0;
    uint8_t working_directory_flag = 0;
  };

  struct new_udp_stream_binding_request : other_message_spec_impl<new_udp_stream_binding_request> {
    binding_point address;
    binding_point remote_address;
  };

  /// response messages
  struct session_check_in_response : other_message_spec_impl<session_check_in_response> {
    integer_t session_id = 0;
  };

  struct session_information_response : other_message_spec_impl<session_information_response> {
    uint8_t project_data_flag = 0;

    uint8_t name_flag = 0;
    std::string name;

    uint8_t executable_flag = 0;
    std::string executable;

    uint8_t working_directory_flag = 0;
    std::string working_directory;

    static std::vector<uint8_t> custom_builder(session_information_response* msg);
    static session_information_response custom_parser(const std::span<const uint8_t> data);
  };

  struct udp_handle;

  struct new_udp_stream_binding_response : other_message_spec_impl<new_udp_stream_binding_response> {
    uint8_t ack_nack = 0;
    integer_t binding_id = 0;
  };

  /// session event messages
  struct session_event_rx_message : other_message_spec_impl<session_event_rx_message> {
    integer_t session_id = 0;
    message msg;

    static std::vector<uint8_t> custom_builder(session_event_rx_message* msg);
    static session_event_rx_message custom_parser(const std::span<const uint8_t> data);
  };

  /// information
  struct udp_binding_information : other_message_spec_impl<udp_binding_information> {
    binding_point endpoint;
    binding_point remote_endpoint;
    uint64_t check_in_hash = 0;
  };

  /// error alert messages

#pragma pack(pop)
}  // namespace other

OTHER_REFLECT(
  other::notification_session_check_in,
  field(session_id)
);

OTHER_REFLECT(
  other::notification_session_closed,
  field(session_id)
);

OTHER_REFLECT(
  other::control_ping,
  field(session_id)
);

OTHER_REFLECT(
  other::control_pong,
  field(session_id)
);

OTHER_REFLECT(
  other::command_session_connect_to,
  field(address)
);

OTHER_REFLECT(
  other::command_session_listen_at,
  field(address)
);

OTHER_REFLECT(
  other::command_session_check_in_at,
  field(session_id),
  field(address)
)

OTHER_REFLECT(
  other::session_check_in_request,
  field(session_id)
);

OTHER_REFLECT(
  other::session_information_request,
  field(project_data_flag),
  field(name_flag),
  field(executable_flag),
  field(working_directory_flag)
);

OTHER_REFLECT(
  other::new_udp_stream_binding_request,
  field(address),
  field(remote_address)
);

OTHER_REFLECT(
  other::new_udp_stream_binding_response,
  field(ack_nack),
  field(binding_id)
)

OTHER_REFLECT(
  other::session_check_in_response,
  field(session_id)
);

OTHER_REFLECT(
  other::udp_binding_information,
  field(endpoint),
  field(remote_endpoint),
  field(check_in_hash)
);

#endif  // OTHER_NETWORK_NETWORK_MESSAGE_HPP