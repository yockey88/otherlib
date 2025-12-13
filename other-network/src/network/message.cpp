/**
 * \file network/message.cpp
 **/
#include "network/message.hpp"

namespace other {

  std::vector<uint8_t> notification_stream_rx_datagram::custom_builder(notification_stream_rx_datagram* msg) {
    std::vector<uint8_t> data;

    const uint8_t* stream_id_bytes = reinterpret_cast<const uint8_t*>(&msg->stream_id);
    const uint8_t* datagram_type_bytes = reinterpret_cast<const uint8_t*>(&msg->datagram.type);
    data.append_range(std::span(stream_id_bytes, sizeof(integer_t)));
    data.append_range(std::span(datagram_type_bytes, sizeof(uint8_t)));

    const uint8_t* datagram_payload_bytes = nullptr;
    switch (msg->datagram.type) {
      case udp_packet_type::UDP_CHECK_IN:
        datagram_payload_bytes = reinterpret_cast<const uint8_t*>(&msg->datagram.packet.check_in);
        data.append_range(std::span(datagram_payload_bytes, sizeof(udp_check_in)));
        break;
      default:
        OTHER_ASSERT(false, "Unknown UDP packet type in notification_stream_rx_datagram::custom_builder");
    }

    return data;
  }

  notification_stream_rx_datagram notification_stream_rx_datagram::custom_parser(const std::span<const uint8_t> data) {
    OTHER_ASSERT(data.size() >= sizeof(integer_t), "Invalid stream rx datagram message size");

    auto bytes = std::span(data);
    integer_t stream_id = *reinterpret_cast<const integer_t*>(bytes.data());

    bytes = bytes.subspan(sizeof(integer_t));
    udp_packet_type datagram_type = static_cast<udp_packet_type>(bytes[0]);

    notification_stream_rx_datagram msg;
    msg.stream_id = stream_id;
    msg.datagram.type = datagram_type;

    bytes = bytes.subspan(sizeof(uint8_t));
    switch (msg.datagram.type) {
      case udp_packet_type::UDP_CHECK_IN:
        OTHER_ASSERT(bytes.size() >= sizeof(udp_check_in), "Invalid UDP check-in packet size");
        msg.datagram.packet.check_in = *reinterpret_cast<const udp_check_in*>(bytes.data());
        bytes = bytes.subspan(sizeof(udp_check_in));
        break;
      default:
        OTHER_ASSERT(false, "Unknown UDP packet type in notification_stream_rx_datagram::custom_parser");
    }

    return msg;
  }

  std::vector<uint8_t> command_session_tx_message::custom_builder(command_session_tx_message* msg) {
    std::vector<uint8_t> data;

    const uint8_t* session_id_bytes = reinterpret_cast<const uint8_t*>(&msg->session_id);
    const uint8_t* session_msg_header_bytes = reinterpret_cast<const uint8_t*>(&msg->msg.header);
    const uint8_t* session_msg_data_bytes = reinterpret_cast<const uint8_t*>(msg->msg.data.data());
    data.append_range(std::span(session_id_bytes, sizeof(integer_t)));
    data.append_range(std::span(session_msg_header_bytes, sizeof(message_header)));
    data.append_range(std::span(session_msg_data_bytes, msg->msg.data.size()));
    return data;
  }

  command_session_tx_message command_session_tx_message::custom_parser(const std::span<const uint8_t> data) {
    OTHER_ASSERT(data.size() >= sizeof(integer_t), "Invalid session tx message size");

    auto bytes = std::span(data);
    integer_t session_id = *reinterpret_cast<const integer_t*>(bytes.data());

    bytes = bytes.subspan(sizeof(integer_t));
    message_header msg_header = *reinterpret_cast<const message_header*>(bytes.data());

    command_session_tx_message msg;
    msg.session_id = session_id;
    msg.msg.header = msg_header;
    msg.msg.data.append_range(bytes.subspan(sizeof(message_header)));

    return msg;
  }

  std::vector<uint8_t> command_load_empty_scene::custom_builder(command_load_empty_scene* msg) {
    std::vector<uint8_t> data;

    uint16_t scene_name_len = static_cast<uint16_t>(msg->scene_name.size());
    const uint8_t* session_id_bytes = reinterpret_cast<const uint8_t*>(&msg->session_id);
    const uint8_t* scene_name_len_bytes = reinterpret_cast<const uint8_t*>(&scene_name_len);
    const uint8_t* scene_name_bytes = reinterpret_cast<const uint8_t*>(msg->scene_name.data());
    if (msg->session_id_flag) {
      data.push_back(0x01);
      data.append_range(std::span(session_id_bytes, sizeof(integer_t)));
    } else {
      data.push_back(0x00);
    }

    if (msg->requires_udp_binding) {
      data.push_back(0x01);
      const uint8_t* udp_address_bytes = reinterpret_cast<const uint8_t*>(&msg->udp_address);
      const uint8_t* server_udp_address_bytes = reinterpret_cast<const uint8_t*>(&msg->server_udp_address);
      data.append_range(std::span(udp_address_bytes, sizeof(binding_point)));
      data.append_range(std::span(server_udp_address_bytes, sizeof(binding_point)));
    } else {
      data.push_back(0x00);
    }

    data.append_range(std::span(scene_name_len_bytes, sizeof(uint16_t)));
    data.append_range(std::span(scene_name_bytes, scene_name_len));

    return data;
  }

  command_load_empty_scene command_load_empty_scene::custom_parser(const std::span<const uint8_t> data) {
    command_load_empty_scene msg;

    auto bytes = std::span(data);

    uint8_t session_id_flag = bytes[0];
    bytes = bytes.subspan(1);

    msg.session_id_flag = session_id_flag;
    if (session_id_flag) {
      msg.session_id = *reinterpret_cast<const integer_t*>(bytes.data());
      bytes = bytes.subspan(sizeof(integer_t));
    }

    uint8_t requires_udp_binding = bytes[0];
    bytes = bytes.subspan(1);
    msg.requires_udp_binding = requires_udp_binding;
    if (requires_udp_binding) {
      msg.udp_address = *reinterpret_cast<const binding_point*>(bytes.data());
      bytes = bytes.subspan(sizeof(binding_point));
      msg.server_udp_address = *reinterpret_cast<const binding_point*>(bytes.data());
      bytes = bytes.subspan(sizeof(binding_point));
    }

    uint16_t scene_name_len = *reinterpret_cast<const uint16_t*>(bytes.data());
    bytes = bytes.subspan(sizeof(uint16_t));
    OTHER_ASSERT(bytes.size() >= scene_name_len, "Invalid load empty scene message size");

    msg.scene_name = std::string(reinterpret_cast<const char*>(bytes.data()), scene_name_len);

    return msg;
  }

  std::vector<uint8_t> command_stream_send_udp_datagram::custom_builder(command_stream_send_udp_datagram* msg) {
    std::vector<uint8_t> data;

    const uint8_t* stream_id_bytes = reinterpret_cast<const uint8_t*>(&msg->stream_id);
    const uint8_t* datagram_type_bytes = reinterpret_cast<const uint8_t*>(&msg->datagram.type);
    data.append_range(std::span(stream_id_bytes, sizeof(integer_t)));
    data.append_range(std::span(datagram_type_bytes, sizeof(uint8_t)));

    const uint8_t* datagram_payload_bytes = nullptr;
    switch (msg->datagram.type) {
      case udp_packet_type::UDP_CHECK_IN:
        datagram_payload_bytes = reinterpret_cast<const uint8_t*>(&msg->datagram.packet.check_in);
        data.append_range(std::span(datagram_payload_bytes, sizeof(udp_check_in)));
        break;
      default:
        OTHER_ASSERT(false, "Unknown UDP packet type in command_stream_send_udp_datagram::custom_builder");
    }

    return data;
  }

  command_stream_send_udp_datagram command_stream_send_udp_datagram::custom_parser(const std::span<const uint8_t> data) {
    OTHER_ASSERT(data.size() >= sizeof(integer_t), "Invalid stream send udp datagram message size");

    auto bytes = std::span(data);
    integer_t stream_id = *reinterpret_cast<const integer_t*>(bytes.data());
    bytes = bytes.subspan(sizeof(integer_t));

    command_stream_send_udp_datagram msg;
    msg.stream_id = stream_id;
    msg.datagram.type = static_cast<udp_packet_type>(bytes[0]);

    bytes = bytes.subspan(sizeof(uint8_t));
    switch (msg.datagram.type) {
      case udp_packet_type::UDP_CHECK_IN:
        OTHER_ASSERT(bytes.size() >= sizeof(udp_check_in), "Invalid UDP check-in packet size");
        msg.datagram.packet.check_in = *reinterpret_cast<const udp_check_in*>(bytes.data());
        bytes = bytes.subspan(sizeof(udp_check_in));
        break;
      default:
        OTHER_ASSERT(false, "Unknown UDP packet type in command_stream_send_udp_datagram::custom_parser");
    }

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

  std::vector<uint8_t> session_event_rx_message::custom_builder(session_event_rx_message* msg) {
    std::vector<uint8_t> data;

    const uint8_t* session_id_bytes = reinterpret_cast<const uint8_t*>(&msg->session_id);
    const uint8_t* session_msg_header_bytes = reinterpret_cast<const uint8_t*>(&msg->msg.header);
    const uint8_t* session_msg_data_bytes = reinterpret_cast<const uint8_t*>(msg->msg.data.data());
    data.append_range(std::span(session_id_bytes, sizeof(integer_t)));
    data.append_range(std::span(session_msg_header_bytes, sizeof(message_header)));
    data.append_range(std::span(session_msg_data_bytes, msg->msg.data.size()));
    return data;
  }

  session_event_rx_message session_event_rx_message::custom_parser(const std::span<const uint8_t> data) {
    OTHER_ASSERT(data.size() >= sizeof(integer_t) + sizeof(message_header), "Invalid session event rx message size");

    auto bytes = std::span(data);
    integer_t session_id = *reinterpret_cast<const integer_t*>(bytes.data());

    bytes = bytes.subspan(sizeof(integer_t));
    message_header msg_header = *reinterpret_cast<const message_header*>(bytes.data());

    session_event_rx_message msg;
    msg.session_id = session_id;
    msg.msg.header = msg_header;
    msg.msg.data.append_range(bytes.subspan(sizeof(message_header)));

    return msg;
  }

}  // namespace other