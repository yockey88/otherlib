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
    auto root = flexbuffers::GetRoot(data);
    auto map = root.AsMap();

    session_status_request msg;

    msg.session_type = map[message_fields[SESSION_TYPE_FIELD].name].AsUInt16();
    msg.node_id = map[message_fields[NODE_ID_FIELD].name].AsUInt64();
    msg.layer_type = map[message_fields[LAYER_TYPE_FIELD].name].AsUInt8();

    return msg;
  }

  std::vector<uint8_t> session_status_request::build() {
    flexbuffers::Builder builder;
    builder.Map([&]() {
      builder.UInt(message_fields[SESSION_TYPE_FIELD].name, session_type);
      builder.UInt(message_fields[NODE_ID_FIELD].name, node_id);
      builder.UInt(message_fields[LAYER_TYPE_FIELD].name, layer_type);
    });
    builder.Finish();

    std::vector<uint8_t> data;
    auto& buffer = builder.GetBuffer();
    data.resize(2 + buffer.size());

    write_header(data, { category, id });
    data.insert(data.end(), buffer.begin(), buffer.end());

    return data;
  }

  std::string session_status_request::write_string(const session_status_request& msg) {
    std::stringstream ss;
    ss << std::format("[status request]: session_type: {:#06x}, node_id: {},", msg.session_type, msg.node_id);
    ss << std::format(" layer_type: {:#04x}", msg.layer_type);
    return ss.str();
  }

  session_status_response session_status_response::parse(const std::vector<uint8_t>& data) {
    auto root = flexbuffers::GetRoot(data);
    auto map = root.AsMap();

    session_status_response msg;
    msg.session_type = map[message_fields[SESSION_TYPE_FIELD].name].AsUInt16();
    msg.node_id = map[message_fields[NODE_ID_FIELD].name].AsUInt64();
    msg.status = map[message_fields[STATUS_FIELD].name].AsUInt64();

    return msg;
  }

  std::vector<uint8_t> session_status_response::build() {
    flexbuffers::Builder builder;
    builder.Map([&]() {
      builder.UInt(message_fields[SESSION_TYPE_FIELD].name, session_type);
      builder.UInt(message_fields[NODE_ID_FIELD].name, node_id);
      builder.UInt(message_fields[STATUS_FIELD].name, status);
    });
    builder.Finish();

    std::vector<uint8_t> data;
    auto& buffer = builder.GetBuffer();
    data.resize(2 + buffer.size());

    write_header(data, { category, id });
    data.insert(data.end(), buffer.begin(), buffer.end());

    return data;
  }

  std::string session_status_response::write_string(const session_status_response& msg) {
    std::stringstream ss;
    ss << std::format("[status response]: session_type: {:#06x}, node_id: {}, status: {}", msg.session_type, msg.node_id, msg.status);
    return ss.str();
  }

  session_shutdown_request session_shutdown_request::parse(const std::vector<uint8_t>& data) {
    auto root = flexbuffers::GetRoot(data);
    auto map = root.AsMap();

    session_shutdown_request msg;

    msg.session_type = map[message_fields[SESSION_TYPE_FIELD].name].AsUInt16();
    msg.node_id = map[message_fields[NODE_ID_FIELD].name].AsUInt64();
    msg.status = map[message_fields[STATUS_FIELD].name].AsUInt64();

    return msg;
  }

  std::vector<uint8_t> session_shutdown_request::build() {
    flexbuffers::Builder builder;
    builder.Map([&]() {
      builder.UInt(message_fields[SESSION_TYPE_FIELD].name, session_type);
      builder.UInt(message_fields[NODE_ID_FIELD].name, node_id);
      builder.UInt(message_fields[STATUS_FIELD].name, status);
    });
    builder.Finish();

    std::vector<uint8_t> data;
    auto& buffer = builder.GetBuffer();
    data.resize(2 + buffer.size());

    write_header(data, { category, id });
    data.insert(data.end(), buffer.begin(), buffer.end());

    return data;
  }

  std::string session_shutdown_request::write_string(const session_shutdown_request& msg) {
    std::stringstream ss;
    ss << std::format("[shutdown request]: session_type: {:#06x}, node_id: {}, status: {}", msg.session_type, msg.node_id, msg.status);
    return ss.str();
  }

  error_alert_msg error_alert_msg::parse(const std::vector<uint8_t>& data) {
    auto root = flexbuffers::GetRoot(data);
    auto map = root.AsMap();

    error_alert_msg msg;
    msg.error_code = map[message_fields[ERROR_CODE_FIELD].name].AsUInt16();
    msg.error_message = map[message_fields[ERROR_MESSAGE_FIELD].name].AsString().str();

    return msg;
  }

  std::vector<uint8_t> error_alert_msg::build() {
    flexbuffers::Builder builder;
    builder.Map([&]() {
      builder.UInt(message_fields[ERROR_CODE_FIELD].name, error_code);
      builder.String(message_fields[ERROR_MESSAGE_FIELD].name, error_message);
    });
    builder.Finish();

    std::vector<uint8_t> data;
    auto& buffer = builder.GetBuffer();
    data.resize(2 + buffer.size());

    write_header(data, { category, id });
    data.insert(data.end(), buffer.begin(), buffer.end());
    return data;
  }

}  // namespace other