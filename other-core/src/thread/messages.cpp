/**
 * \file thread/messages.cpp
 **/
#include "thread/messages.hpp"

#include <flatbuffers/flexbuffers.h>

namespace other {

  std::vector<uint8_t> load_empty_scene_command::custom_builder(load_empty_scene_command* msg) {
    std::vector<uint8_t> data;

    natural_t scene_name_len = static_cast<natural_t>(msg->scene_name.size());
    const uint8_t* session_id_bytes = reinterpret_cast<const uint8_t*>(&msg->session_id);
    const uint8_t* scene_name_len_bytes = reinterpret_cast<const uint8_t*>(&scene_name_len);
    const uint8_t* scene_name_bytes = reinterpret_cast<const uint8_t*>(msg->scene_name.data());
    if (msg->session_id_flag) {
      data.push_back(0x01);
      data.append_range(std::span(session_id_bytes, sizeof(integer_t)));
    } else {
      data.push_back(0x00);
    }
    data.append_range(std::span(scene_name_len_bytes, sizeof(natural_t)));
    data.append_range(std::span(scene_name_bytes, scene_name_len));

    return data;
  }

  load_empty_scene_command load_empty_scene_command::custom_parser(const std::span<const uint8_t> data) {
    load_empty_scene_command msg;
    size_t cursor = 0;

    uint8_t session_id_flag = data[cursor];
    cursor += sizeof(uint8_t);

    msg.session_id_flag = session_id_flag;
    if (session_id_flag) {
      msg.session_id = *reinterpret_cast<const integer_t*>(data.data() + cursor);
      cursor += sizeof(integer_t);
    }

    natural_t scene_name_len = *reinterpret_cast<const natural_t*>(data.data() + cursor);
    cursor += sizeof(natural_t);
    msg.scene_name = std::string(reinterpret_cast<const char*>(data.data() + cursor), scene_name_len);
    cursor += scene_name_len;

    return msg;
  }

  std::vector<uint8_t> session_information_response::custom_builder(session_information_response* msg) {
    std::vector<uint8_t> data;

    data.push_back(msg->project_data_flag);
    data.push_back(msg->name_flag);
    if (msg->name_flag) {
      natural_t name_len = static_cast<natural_t>(msg->name.size());
      const uint8_t* name_len_bytes = reinterpret_cast<const uint8_t*>(&name_len);
      const uint8_t* name_bytes = reinterpret_cast<const uint8_t*>(msg->name.data());
      data.append_range(std::span(name_len_bytes, sizeof(natural_t)));
      data.append_range(std::span(name_bytes, name_len));
    }

    data.push_back(msg->executable_flag);
    if (msg->executable_flag) {
      natural_t executable_len = static_cast<natural_t>(msg->executable.size());
      const uint8_t* executable_len_bytes = reinterpret_cast<const uint8_t*>(&executable_len);
      const uint8_t* executable_bytes = reinterpret_cast<const uint8_t*>(msg->executable.data());
      data.append_range(std::span(executable_len_bytes, sizeof(natural_t)));
      data.append_range(std::span(executable_bytes, executable_len));
    }

    data.push_back(msg->working_directory_flag);
    if (msg->working_directory_flag) {
      natural_t working_directory_len = static_cast<natural_t>(msg->working_directory.size());
      const uint8_t* working_directory_len_bytes = reinterpret_cast<const uint8_t*>(&working_directory_len);
      const uint8_t* working_directory_bytes = reinterpret_cast<const uint8_t*>(msg->working_directory.data());
      data.append_range(std::span(working_directory_len_bytes, sizeof(natural_t)));
      data.append_range(std::span(working_directory_bytes, working_directory_len));
    }

    return data;
  }

  session_information_response session_information_response::custom_parser(const std::span<const uint8_t> data) {
    session_information_response msg;
    size_t cursor = 0;

    msg.project_data_flag = data[cursor];
    cursor += sizeof(uint8_t);

    msg.name_flag = data[cursor];
    cursor++;

    if (msg.name_flag) {
      natural_t name_len = *reinterpret_cast<const natural_t*>(data.data() + cursor);
      cursor += sizeof(natural_t);
      msg.name = std::string(reinterpret_cast<const char*>(data.data() + cursor), name_len);
      cursor += name_len;
    }

    msg.executable_flag = data[cursor];
    cursor++;
    if (msg.executable_flag) {
      natural_t executable_len = *reinterpret_cast<const natural_t*>(data.data() + cursor);
      cursor += sizeof(natural_t);
      msg.executable = std::string(reinterpret_cast<const char*>(data.data() + cursor), executable_len);
      cursor += executable_len;
    }

    msg.working_directory_flag = data[cursor];
    cursor++;
    if (msg.working_directory_flag) {
      natural_t working_directory_len = *reinterpret_cast<const natural_t*>(data.data() + cursor);
      cursor += sizeof(natural_t);
      msg.working_directory = std::string(reinterpret_cast<const char*>(data.data() + cursor), working_directory_len);
      cursor += working_directory_len;
    }

    return msg;
  }

  acknowledgement acknowledgement::parse(const std::vector<uint8_t>& data) {
    auto root = flexbuffers::GetRoot(data);
    auto map = root.AsMap();
    auto header = map[message_fields[ACKED_HEADER_FIELD].name].AsMap();

    acknowledgement msg;
    msg.acked_header.category = static_cast<message_category>(header[message_fields[MSG_CATEGORY_FIELD].name].AsUInt8());
    msg.acked_header.id = static_cast<message_id>(header[message_fields[MSG_ID_FIELD].name].AsUInt8());
    msg.ack_nack = map[message_fields[ACK_NACK_FIELD].name].AsUInt8();

    return msg;
  }

  std::vector<uint8_t> acknowledgement::build() {
    flexbuffers::Builder builder;
    builder.Map([&]() {
      builder.Map(message_fields[ACKED_HEADER_FIELD].name, [&]() {
        builder.UInt(message_fields[MSG_CATEGORY_FIELD].name, acked_header.category);
        builder.UInt(message_fields[MSG_ID_FIELD].name, acked_header.id);
      });
      builder.Int(message_fields[ACK_NACK_FIELD].name, ack_nack);
    });
    builder.Finish();

    std::vector<uint8_t> data;
    auto& buffer = builder.GetBuffer();
    data.resize(2 + buffer.size());

    write_header(data, { category, id });
    data.insert(data.end(), builder.GetBuffer().begin(), builder.GetBuffer().end());

    return data;
  }

  std::string acknowledgement::write_string(const acknowledgement& msg) {
    std::stringstream ss;
    ss << "[acknowledgment]: acked_header: "
       << std::format("{{ category: {}, msg-id: {}, ack: {} }}", (message_category)msg.acked_header.category, (message_id)msg.acked_header.id, (uint32_t)msg.ack_nack)
       << ", node_id: " << msg.node_id;
    return ss.str();
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