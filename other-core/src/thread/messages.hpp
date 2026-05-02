/**
 * \file thread/messages.hpp
 **/
#ifndef OTHER_CORE_CORE_MESSAGES_HPP
#define OTHER_CORE_CORE_MESSAGES_HPP

#include "thread/message.hpp"
#include "thread/test/new_message.hpp"

namespace other {

#pragma pack(push, 1)

  struct message_parsing_error : public std::runtime_error {
    message_parsing_error(const std::string& msg)
        : std::runtime_error(msg) {}
  };

  /// acknowledgement messages
  struct acknowledgement : other_message_spec_impl<acknowledgement> {
    integer_t session_id = -1;
    message_header acked_header;
    uint8_t ack_nack = 0;

    uint16_t extra_data_length = 0;
    std::vector<uint8_t> extra_data;

    static std::vector<uint8_t> custom_builder(acknowledgement* msg);
    static acknowledgement custom_parser(const std::span<const uint8_t> data);
  };

  /// \todo update all messages to new format, ditch flexbuffers for messaging system

  /// OLD BEGIN //////////////////////////////////////////////////////////////////////////////
  /// notification messages
  /// control messages
  struct session_status_request : message_spec_impl<session_status_request> {
    constexpr static message_category category = CONTROL;
    constexpr static message_id id = PING;

    uint16_t session_type = 0;
    uint64_t node_id = 0;
    uint8_t layer_type = 0;

    static session_status_request parse(const std::vector<uint8_t>& data);
    std::vector<uint8_t> build();
    static std::string write_string(const session_status_request& msg);
  };

  struct session_status_response : message_spec_impl<session_status_response> {
    constexpr static message_category category = CONTROL;
    constexpr static message_id id = PONG;

    uint16_t session_type = 0;
    uint64_t node_id = 0;
    uint64_t status = 0;

    static session_status_response parse(const std::vector<uint8_t>& data);
    std::vector<uint8_t> build();
    static std::string write_string(const session_status_response& msg);
  };

  /// request messages
  /// response messages
  /// session event messages

  /// error alert messages
  struct error_alert_msg : message_spec_impl<error_alert_msg> {
    constexpr static message_category category = ERROR_ALERT;
    constexpr static message_id id = ERROR_ALERT_ID;

    uint64_t error_code = 0;
    std::string error_message;

    static error_alert_msg parse(const std::vector<uint8_t>& data);
    std::vector<uint8_t> build();
  };
  /// OLD END ////////////////////////////////////////////////////////////////////////////////

#pragma pack(pop)

}  // namespace other

OTHER_REFLECT(
  other::acknowledgement,
  field(acked_header),
  field(ack_nack)
);

#endif  // OTHER_CORE_CORE_MESSAGES_HPP