/**
 * \file thread/message.cpp
 **/
#include "thread/message.hpp"

#include <format>
#include <sstream>

#include <flatbuffers/flexbuffers.h>

#include "core/defines.hpp"
#include "core/logger.hpp"

namespace other {

  std::string binding_point::write_string(const binding_point& bp) {
    std::stringstream ss;
    ss << "Binding point: ";
    for (integer_t i = 3; i >= 0; --i) {
      ss << std::to_string(bp.bytes[i]);
      if (i != 0) {
        ss << ".";
      }
    }
    ss << ":" << bp.port;
    return ss.str();
  }

  binding_point binding_point::from_asio(const asio::ip::address& addr, uint16_t port) {
    binding_point bp;
    bp.port = port;
    if (addr.is_v4()) {
      bp.ip = addr.to_v4().to_uint();
    } else {
      OTHER_ASSERT(false, "non IPv4 IP addresses not supported currently");
    }
    return bp;
  }

  /// NOTE: we could do better, there is still a lot of duplication here...
  ///         look into using the fields defined in the header to make it better

  void message_spec::write_header(std::vector<uint8_t>& data, const message_header& header) {
    data.resize(2);
    data[0] = static_cast<uint8_t>(header.category);
    data[1] = static_cast<uint8_t>(header.id);
  }

  acknowledgement acknowledgement::parse(const std::vector<uint8_t>& data) {
    auto root = flexbuffers::GetRoot(data);
    auto map = root.AsMap();
    auto header = map[message_fields[ACKED_HEADER_FIELD].name].AsMap();

    acknowledgement msg;
    msg.acked_header.category = static_cast<message_category>(header[message_fields[MSG_CATEGORY_FIELD].name].AsUInt8());
    msg.acked_header.id = static_cast<message_id>(header[message_fields[MSG_ID_FIELD].name].AsUInt8());
    msg.ack_nack = map[message_fields[STATUS_FIELD].name].AsUInt8();

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
    write_header(data, { category, id });
    data.insert(data.end(), builder.GetBuffer().begin(), builder.GetBuffer().end());

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
    write_header(data, { category, id });
    data.insert(data.end(), builder.GetBuffer().begin(), builder.GetBuffer().end());

    return data;
  }

  simulation_description simulation_description::parse(const std::vector<uint8_t>& data) {
    auto root = flexbuffers::GetRoot(data);
    auto map = root.AsMap();

    simulation_description msg;
    msg.session_type = map[message_fields[SESSION_TYPE_FIELD].name].AsUInt16();
    msg.node_id = map[message_fields[NODE_ID_FIELD].name].AsUInt64();
    msg.status = map[message_fields[STATUS_FIELD].name].AsUInt64();

    auto sim_bp = map[message_fields[SIMULATION_BINDING_POINT_FIELD].name].AsMap();
    msg.simulation_binding_point.port = sim_bp["port"].AsUInt16();
    msg.simulation_binding_point.ip = sim_bp["ip"].AsUInt32();

    return msg;
  }

  std::vector<uint8_t> simulation_description::build() {
    flexbuffers::Builder builder;
    builder.Map([&]() {
      builder.UInt(message_fields[SESSION_TYPE_FIELD].name, session_type);
      builder.UInt(message_fields[NODE_ID_FIELD].name, node_id);
      builder.UInt(message_fields[STATUS_FIELD].name, status);
      builder.Map(message_fields[SIMULATION_BINDING_POINT_FIELD].name, [&]() {
        builder.UInt(message_fields[PORT_FIELD].name, simulation_binding_point.port);
        builder.UInt(message_fields[IP_FIELD].name, simulation_binding_point.ip);
      });
    });
    builder.Finish();

    std::vector<uint8_t> data;
    write_header(data, { category, id });
    data.insert(data.end(), builder.GetBuffer().begin(), builder.GetBuffer().end());

    return data;
  }

  std::string simulation_description::write_string(const simulation_description& msg) {
    std::stringstream ss;
    ss << std::format("[simulation description]: session_type: {:#06x}, node_id: {}, status: {},\n", msg.session_type, msg.node_id, msg.status);
    ss << std::format("                                        simulation_binding_point: {{ port: {}, ip: {} }}", msg.simulation_binding_point.port, msg.simulation_binding_point.ip);
    return ss.str();
  }

  std::string session_status_response::write_string(const session_status_response& msg) {
    std::stringstream ss;
    ss << std::format("[status response]: session_type: {:#06x}, node_id: {}, status: {}", msg.session_type, msg.node_id, msg.status);
    return ss.str();
  }

  node_initialization_request node_initialization_request::parse(const std::vector<uint8_t>& data) {
    auto root = flexbuffers::GetRoot(data);
    auto map = root.AsMap();

    node_initialization_request msg;
    msg.session_type = map["session-type"].AsUInt16();
    msg.node_id = map["node-id"].AsUInt64();
    // msg.layer_type = map["layer-type"].AsUInt8();

    return msg;
  }

  std::vector<uint8_t> node_initialization_request::build() {
    flexbuffers::Builder builder;
    builder.Map([&]() {
      builder.UInt(message_fields[SESSION_TYPE_FIELD].name, session_type);
      builder.UInt(message_fields[NODE_ID_FIELD].name, node_id);

      builder.Map(message_fields[COMM_LAYER_BINDING_POINT_FIELD].name, [&]() {
        builder.UInt(message_fields[PORT_FIELD].name, comm_layer_endpoint.port);
        builder.UInt(message_fields[IP_FIELD].name, comm_layer_endpoint.ip);
      });

      builder.Map(message_fields[ANALYTIC_LAYER_BINDING_POINT_FIELD].name, [&]() {
        builder.UInt(message_fields[PORT_FIELD].name, analytics_layer_endpoint.port);
        builder.UInt(message_fields[IP_FIELD].name, analytics_layer_endpoint.ip);
      });
    });
    builder.Finish();

    std::vector<uint8_t> data;
    write_header(data, { category, id });
    data.insert(data.end(), builder.GetBuffer().begin(), builder.GetBuffer().end());

    return data;
  }

  std::string node_initialization_request::write_string(const node_initialization_request& msg) {
    std::stringstream ss;
    // ss << "[node initialization request]: session_type: {:#06x}, node_id: {}, layer_type: {:#04x}";
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
    write_header(data, { category, id });
    data.insert(data.end(), builder.GetBuffer().begin(), builder.GetBuffer().end());

    return data;
  }

  std::string session_shutdown_request::write_string(const session_shutdown_request& msg) {
    std::stringstream ss;
    ss << std::format("[shutdown request]: session_type: {:#06x}, node_id: {}, status: {}", msg.session_type, msg.node_id, msg.status);
    return ss.str();
  }

}  // namespace other