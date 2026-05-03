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
#include "serialization/reflection.hpp"
#include "thread/channel.hpp"

namespace other {

  enum message_category : uint16_t {
    NOTIFICATION = 0,
    ACKNOWLEDGEMENT,

    CONTROL,
    COMMAND,

    REQUEST,
    RESPONSE,

    INFORMATION,

    ERROR_ALERT,
  };

  enum message_id : uint16_t {

    /// ack messages
    ACK = 0x0001,

    /// notification messages
    NETWORK_THREAD_READY,
    NETWORK_THREAD_SHUTDOWN_COMPLETE,
    NEW_TCP_CONNECTION_ACCEPTED,

    /// control messages
    PING,
    PONG,
    VERSION_HANDSHAKE,

    /// command messages
    LISTEN_TCP_CONNECTION,
    CONNECT_TCP_CONNECTION,
    OPEN_UDP_CONNECTION,
    CLOSE_CONNECTION,

    /// request/response messages
    /// error alert messages

    SHUTDOWN_REQUEST,
    ERROR_ALERT_ID = 0xFFFF,
  };

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
#pragma pack(pop)

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
  };

  using message_channel = channel<message>;

}  // namespace other

OTHER_REFLECT(
  other::message_header,
  field(category, other::attr::serializable("category")),
  field(id, other::attr::serializable("id"))
)

OTHER_REFLECT(
  other::binding_point,
  field(port, other::attr::serializable("port")),
  field(ip, other::attr::serializable("ip"))
)

OTHER_REFLECT(
  other::version,
  field(major, other::attr::serializable("major")),
  field(minor, other::attr::serializable("minor")),
  field(patch, other::attr::serializable("patch"))
)

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
      const std::string fmt = std::format("[{}.{}]", other::message_category{ header.category }, other::message_id{ header.id });
      return formatter<std::string_view>::format(fmt, ctx);
    }
  };

}  // namespace std

#endif  // OTHERLIB_THREAD_MESSAGE_HPP