/**
 * \file thread/messages.hpp
 **/
#ifndef OTHER_CORE_CORE_MESSAGES_HPP
#define OTHER_CORE_CORE_MESSAGES_HPP

#include "core/defines.hpp"
#include "thread/message.hpp"
#include "thread/test/new_message.hpp"

#pragma pack(push, 1)
namespace other {

  /// notification messages
  /// acknowledgement messages
  /// control messages
  /// command messages
  struct load_empty_scene_command : other_message_spec_impl<load_empty_scene_command> {
    uint8_t session_id_flag = 0;
    integer_t session_id = 0;
    std::string scene_name;

    static std::vector<uint8_t> custom_builder(load_empty_scene_command* msg);
    static load_empty_scene_command custom_parser(const std::span<const uint8_t> data);
  };
  /// request messages
  struct session_information_request : other_message_spec_impl<session_information_request> {
    uint8_t project_data_flag = 0;
    uint8_t name_flag = 0;
    uint8_t executable_flag = 0;
    uint8_t working_directory_flag = 0;
  };

  /// response messages
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

  /// session event messages
  /// error alert messages

  /// \todo update all messages to new format, ditch flexbuffers for messaging system

  /// OLD BEGIN //////////////////////////////////////////////////////////////////////////////
  /// notification messages
  /// acknowledgement messages
  struct acknowledgement : message_spec_impl<acknowledgement> {
    constexpr static message_category category = ACKNOWLEDGEMENT;
    constexpr static message_id id = ACK;

    message_header acked_header;
    uint8_t ack_nack = 0;
    uint64_t node_id = 0;

    static acknowledgement parse(const std::vector<uint8_t>& data);
    std::vector<uint8_t> build();
    static std::string write_string(const acknowledgement& msg);
  };

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
  struct session_shutdown_request : message_spec_impl<session_shutdown_request> {
    constexpr static message_category category = CONTROL;
    constexpr static message_id id = SESSION_SHUTDOWN;

    uint16_t session_type = 0;
    uint64_t node_id = 0;
    uint64_t status = 0;

    static session_shutdown_request parse(const std::vector<uint8_t>& data);
    std::vector<uint8_t> build();
    static std::string write_string(const session_shutdown_request& msg);
  };

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

}  // namespace other
#pragma pack(pop)

#endif  // OTHER_CORE_CORE_MESSAGES_HPP