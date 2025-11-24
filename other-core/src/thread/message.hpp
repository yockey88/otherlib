/**
 * \file thread/message.hpp
 **/
#ifndef OTHER_CORE_CORE_MESSAGE_HPP
#define OTHER_CORE_CORE_MESSAGE_HPP

#include <cstdint>
#include <string>
#include <vector>

#include <spdlog/fmt/fmt.h>

#ifndef ASIO_HAS_STD_INVOKE_RESULT
  #define ASIO_HAS_STD_INVOKE_RESULT
#endif
#include <asio/asio.hpp>

#include "command/command.hpp"
#include "core/registers.hpp"
#include "thread/channel.hpp"

namespace other {

  enum message_category : uint16_t {
    NOTIFICATION = 0,
    ACKNOWLEDGEMENT,

    CONTROL,

    COMMAND,
    REQUEST,
    RESPONSE,

    ERROR_ALERT,

    INFO,
  };

  enum message_id : uint16_t {
    /// notification messages
    /// ack messages
    ACK = 0x0001,

    /// control messages
    PING,
    PONG,

    SESSION_LISTEN_FOR,

    SESSION_CHECK_IN,
    SESSION_CLOSED,
    SESSION_SHUTDOWN_REQUEST,

    OTHER_COMMAND,
    OTHER_COMMAND_BLOCK,

    /// query messages
    /// response messages
    /// error alert messages
    /// info messages

    SHUTDOWN_REQUEST,

    ERROR_ALERT_ID = 0xFFFF,
  };

#pragma pack(push, 1)
  struct message_header {
    uint16_t category;
    uint16_t id;
    constexpr auto operator<=>(const message_header& other) const = default;
  };
#pragma pack(pop)

  struct binding_point {
    uint16_t port = 0;
    union {
      uint32_t ip;
      uint8_t bytes[4] = { 0, 0, 0, 0 };
    };

    constexpr binding_point() = default;
    constexpr binding_point(uint32_t ip, uint16_t port) : port(port), ip(ip) {}

    static std::string write_string(const binding_point& bp);
    static binding_point from_asio(const asio::ip::address& addr, uint16_t port);
  };

  struct session_endpoint {
    binding_point simulation;
    binding_point control;

    static std::string write_string(const session_endpoint& endpoint);
  };

  struct message;

  template <typename T>
  concept message_spec_type = requires(T t) {
    { T::category } -> std::same_as<message_category>;
    { T::id } -> std::same_as<message_id>;
    { T::parse(std::declval<const std::vector<uint8_t>&>()) } -> std::same_as<T>;
    { t.build() } -> std::same_as<std::vector<uint8_t>>;
  };

  template <typename T, typename... Args>
  concept self_building_message = std::constructible_from<T, Args...> && requires(const T& obj, Args&&... args) {
    { obj.build() } -> std::same_as<message>;
  };

  template <typename T>
  concept self_parsing_message = requires(const T& obj) {
    { obj.parse(std::declval<const std::vector<uint8_t>&>()) } -> std::same_as<T>;
  };

  template <typename T>
  struct message_spec_impl;

  struct message_spec {
    virtual ~message_spec() = default;
    virtual std::vector<uint8_t> build_message() = 0;

   protected:
    void write_header(std::vector<uint8_t>& data, const message_header& header);
  };

  template <typename T>
  struct message_spec_impl : message_spec {
    std::vector<uint8_t> build_message() override;
  };

  using network_packet_parse_error = std::runtime_error;

  struct message {
    message_header header;
    std::vector<uint8_t> data;

    void set_category(message_category category) { header.category = category; }
    message_category get_category() const { return (message_category)header.category; }

    void set_id(message_id id) { header.id = id; }
    message_id get_id() const { return (message_id)header.id; }

    message() = default;
    message(message_category category, message_id type) {
      header = { category, type };
    }
    message(message_category category, uint16_t type)
        : header{ category, type } {}

    message(const message_header& msg_header, const std::vector<uint8_t>& msg_data)
        : header(msg_header), data(msg_data) {}

    template <typename T>
      requires self_parsing_message<T>
    T parse_message(const std::vector<uint8_t>& data) const {
      return T::parse(data);
    }
  };

  template <typename T>
  std::vector<uint8_t> message_spec_impl<T>::build_message() {
    return reinterpret_cast<T*>(this)->build();
  }

  using message_channel = channel<message>;

  struct field_bounds {
    natural_t min = 0;
    natural_t max = 0;
  };
  struct message_field {
    const char* name;
    field_bounds size = { 0, 0 };
  };

  constexpr inline message_field message_fields[] = {
    { "msg-category", { sizeof(message_category), sizeof(message_category) } },
    { "msg-id", { sizeof(message_id), sizeof(message_id) } },

    { "session-type", { sizeof(uint16_t), sizeof(uint16_t) } },
    { "node-id", { sizeof(uint64_t), sizeof(uint64_t) } },
    { "layer-type", { 0, sizeof(uint8_t) } },
    { "status", { sizeof(uint64_t), sizeof(uint64_t) } },

    { "acked-header", { sizeof(message_header), sizeof(message_header) } },
    { "ack-nack", { sizeof(uint8_t), sizeof(uint8_t) } },

    { "port", { sizeof(uint16_t), sizeof(uint16_t) } },
    { "ip", { sizeof(uint32_t), sizeof(uint32_t) } },

    { "opcode", { sizeof(uint8_t), sizeof(uint8_t) } },
    { "argc", { sizeof(uint8_t), sizeof(uint8_t) } },
    { "argv", { 0, 0 } },

    { "error-code", { sizeof(uint16_t), sizeof(uint16_t) } },
    { "error-message", { 0, 0 } },
  };

  enum message_field_idx : uint8_t {
    MSG_CATEGORY_FIELD = 0,
    MSG_ID_FIELD,

    SESSION_TYPE_FIELD,
    NODE_ID_FIELD,
    LAYER_TYPE_FIELD,
    STATUS_FIELD,

    ACKED_HEADER_FIELD,
    ACK_NACK_FIELD,

    PORT_FIELD,
    IP_FIELD,

    OPCODE_FIELD,
    ARGC_FIELD,
    ARGV_FIELD,

    ERROR_CODE_FIELD,
    ERROR_MESSAGE_FIELD,
  };

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

  struct session_shutdown_request : message_spec_impl<session_shutdown_request> {
    constexpr static message_category category = CONTROL;
    constexpr static message_id id = SESSION_SHUTDOWN_REQUEST;

    uint16_t session_type = 0;
    uint64_t node_id = 0;
    uint64_t status = 0;

    static session_shutdown_request parse(const std::vector<uint8_t>& data);
    std::vector<uint8_t> build();
    static std::string write_string(const session_shutdown_request& msg);
  };

  /// thread messages have no messages, they are sort of ad-hoc messages

  struct other_command_msg : message_spec_impl<other_command_msg> {
    constexpr static message_category category = COMMAND;
    constexpr static message_id id = OTHER_COMMAND;

    command cmd;
    std::vector<address_t> args;

    static other_command_msg parse(const std::vector<uint8_t>& data);
    std::vector<uint8_t> build();
  };

  struct other_command_block_msg : message_spec_impl<other_command_block_msg> {
    constexpr static message_category category = COMMAND;
    constexpr static message_id id = OTHER_COMMAND_BLOCK;

    command_block block;
    std::vector<address_t> args;

    static other_command_block_msg parse(const std::vector<uint8_t>& data);
    std::vector<uint8_t> build();
  };

  /// various error messages

  struct error_alert_msg : message_spec_impl<error_alert_msg> {
    constexpr static message_category category = ERROR_ALERT;
    constexpr static message_id id = ERROR_ALERT_ID;

    uint64_t error_code = 0;
    std::string error_message;

    static error_alert_msg parse(const std::vector<uint8_t>& data);
    std::vector<uint8_t> build();
  };

}  // namespace other

namespace std {

  template <>
  struct formatter<other::binding_point> : public formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const other::binding_point& bp, FormatContext& ctx) const {
      const std::string fmt = std::format("{}.{}.{}.{}:{}", bp.bytes[0], bp.bytes[1], bp.bytes[2], bp.bytes[3], bp.port);
      return formatter<std::string_view>::format(fmt, ctx);
    }
  };

  template <>
  struct formatter<other::message_header> : public formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const other::message_header& header, FormatContext& ctx) const {
      const std::string fmt = std::format("[{:#06x}:{:#06x}]", header.category, header.id);
      return formatter<std::string_view>::format(fmt, ctx);
    }
  };

}  // namespace std

#endif  // OTHERLIB_THREAD_MESSAGE_HPP