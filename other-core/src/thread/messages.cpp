/**
 * \file thread/messages.cpp
 **/
#include "thread/messages.hpp"

#include <flatbuffers/flexbuffers.h>

namespace other {

  std::vector<uint8_t> acknowledgement::custom_builder(acknowledgement* msg) {
    std::vector<uint8_t> data;

    msg->extra_data_length = static_cast<uint16_t>(msg->extra_data.size());

    const uint8_t* session_id_bytes = reinterpret_cast<const uint8_t*>(&msg->session_id);
    const uint8_t* acked_header_bytes = reinterpret_cast<const uint8_t*>(&msg->acked_header);
    const uint8_t* ack_nack_bytes = reinterpret_cast<const uint8_t*>(&msg->ack_nack);
    const uint8_t* extra_data_length_bytes = reinterpret_cast<const uint8_t*>(&msg->extra_data_length);
    data.append_range(std::span(session_id_bytes, sizeof(integer_t)));
    data.append_range(std::span(acked_header_bytes, sizeof(message_header)));
    data.append_range(std::span(ack_nack_bytes, sizeof(uint8_t)));
    data.append_range(std::span(extra_data_length_bytes, sizeof(uint16_t)));

    CORE_LOG_TRACE("Building acknowledgement message with extra data length: {}", msg->extra_data_length);
    if (!msg->extra_data.empty()) {
      data.append_range(std::span(msg->extra_data.data(), msg->extra_data.size()));
    }

    return data;
  }

  acknowledgement acknowledgement::custom_parser(const std::span<const uint8_t> data) {
    auto bytes = std::span(data);

    integer_t session_id = *reinterpret_cast<const integer_t*>(bytes.data());
    bytes = bytes.subspan(sizeof(integer_t));

    message_header acked_header = *reinterpret_cast<const message_header*>(bytes.data());
    bytes = bytes.subspan(sizeof(message_header));

    uint8_t ack_nack = bytes[0];
    bytes = bytes.subspan(sizeof(uint8_t));

    uint16_t extra_data_length = *reinterpret_cast<const uint16_t*>(bytes.data());
    bytes = bytes.subspan(sizeof(uint16_t));

    CORE_LOG_TRACE("Parsing acknowledgement message with extra data length: {}", extra_data_length);
    if (extra_data_length > 0 && extra_data_length != bytes.size()) {
      CORE_LOG_ERROR("Acknowledgement message extra data length mismatch: expected {}, got {}", extra_data_length, bytes.size());
      throw std::runtime_error("Acknowledgement message extra data length mismatch");
    }

    acknowledgement msg;
    msg.session_id = session_id;
    msg.acked_header = acked_header;
    msg.ack_nack = ack_nack;
    msg.extra_data_length = extra_data_length;

    if (extra_data_length > 0) {
      msg.extra_data.append_range(bytes.subspan(0, extra_data_length));
    }

    return msg;
  }

  session_status_request session_status_request::parse(const std::vector<uint8_t>& data) {
    const uint16_t* session_type_ptr = reinterpret_cast<const uint16_t*>(data.data());
    const uint64_t* node_id_ptr = reinterpret_cast<const uint64_t*>(data.data() + sizeof(uint16_t));
    const uint8_t* layer_type_ptr = reinterpret_cast<const uint8_t*>(data.data() + sizeof(uint16_t) + sizeof(uint64_t));

    session_status_request msg;
    msg.session_type = *session_type_ptr;
    msg.node_id = *node_id_ptr;
    msg.layer_type = *layer_type_ptr;

    return msg;
  }

  std::vector<uint8_t> session_status_request::build() {
    std::vector<uint8_t> data;
    const uint8_t* session_type_bytes = reinterpret_cast<const uint8_t*>(&session_type);
    const uint8_t* node_id_bytes = reinterpret_cast<const uint8_t*>(&node_id);
    const uint8_t* layer_type_bytes = reinterpret_cast<const uint8_t*>(&layer_type);
    data.append_range(std::span(session_type_bytes, sizeof(uint16_t)));
    data.append_range(std::span(node_id_bytes, sizeof(uint64_t)));
    data.append_range(std::span(layer_type_bytes, sizeof(uint8_t)));
    return data;
  }

  std::string session_status_request::write_string(const session_status_request& msg) {
    std::stringstream ss;
    ss << std::format("[status request]: session_type: {:#06x}, node_id: {},", msg.session_type, msg.node_id);
    ss << std::format(" layer_type: {:#04x}", msg.layer_type);
    return ss.str();
  }

  session_status_response session_status_response::parse(const std::vector<uint8_t>& data) {
    const uint16_t* session_type_ptr = reinterpret_cast<const uint16_t*>(data.data());
    const uint64_t* node_id_ptr = reinterpret_cast<const uint64_t*>(data.data() + sizeof(uint16_t));
    const uint64_t* status_ptr = reinterpret_cast<const uint64_t*>(data.data() + sizeof(uint16_t) + sizeof(uint64_t));

    session_status_response msg;
    msg.session_type = *session_type_ptr;
    msg.node_id = *node_id_ptr;
    msg.status = *status_ptr;

    return msg;
  }

  std::vector<uint8_t> session_status_response::build() {
    std::vector<uint8_t> data;
    const uint8_t* session_type_bytes = reinterpret_cast<const uint8_t*>(&session_type);
    const uint8_t* node_id_bytes = reinterpret_cast<const uint8_t*>(&node_id);
    const uint8_t* status_bytes = reinterpret_cast<const uint8_t*>(&status);
    data.append_range(std::span(session_type_bytes, sizeof(uint16_t)));
    data.append_range(std::span(node_id_bytes, sizeof(uint64_t)));
    data.append_range(std::span(status_bytes, sizeof(uint64_t)));
    return data;
  }

  std::string session_status_response::write_string(const session_status_response& msg) {
    std::stringstream ss;
    ss << std::format("[status response]: session_type: {:#06x}, node_id: {}, status: {}", msg.session_type, msg.node_id, msg.status);
    return ss.str();
  }

  error_alert_msg error_alert_msg::parse(const std::vector<uint8_t>& data) {
    const uint16_t* error_code_ptr = reinterpret_cast<const uint16_t*>(data.data());
    const char* error_message_ptr = reinterpret_cast<const char*>(data.data() + sizeof(uint16_t));

    error_alert_msg msg;
    msg.error_code = *error_code_ptr;
    msg.error_message = std::string(error_message_ptr);
    return msg;
  }

  std::vector<uint8_t> error_alert_msg::build() {
    std::vector<uint8_t> data;
    const uint8_t* error_code_bytes = reinterpret_cast<const uint8_t*>(&error_code);
    data.append_range(std::span(error_code_bytes, sizeof(uint16_t)));
    data.append_range(std::span(reinterpret_cast<const uint8_t*>(error_message.data()), error_message.size() + 1));
    return data;
  }

}  // namespace other