/**
 * \file message/message_defines.hpp
 **/
#ifndef OTHER_CORE_MESSAGE_MESSAGE_DEFINES_HPP
#define OTHER_CORE_MESSAGE_MESSAGE_DEFINES_HPP

#include <cstdint>
#include <string>
#include <vector>

#include <spdlog/fmt/fmt.h>

#ifndef ASIO_HAS_STD_INVOKE_RESULT
  #define ASIO_HAS_STD_INVOKE_RESULT
#endif
#include <asio/asio.hpp>

#include "serialization/reflection.hpp"
#include "serialization/serialization.hpp"
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
    RX_DATA,

    /// control messages
    PING,
    PONG,
    VERSION_HANDSHAKE,

    /// command messages
    LISTEN_CONNECTION,
    CONNECT_CONNECTION,
    CLOSE_CONNECTION,

    TX_DATA,
    ATTACH_TRANSPORT_LISTENER,
    DETACH_TRANSPORT_LISTENER,
    ATTACH_CONNECTION_SINK,
    DETACH_CONNECTION_SINK,

    /// request/response messages
    /// error alert messages

    SHUTDOWN_REQUEST,
    ERROR_ALERT_ID = 0xFFFF,
  };

  struct message {
    uint16_t category;
    uint16_t id;
    ostd::vector<uint8_t> data;

    void set_category(message_category category) { this->category = category; }
    message_category get_category() const { return (message_category)this->category; }

    void set_id(message_id id) { this->id = id; }
    message_id get_id() const { return (message_id)this->id; }

    message() = default;
    message(message_category category, message_id type) {
      this->category = category;
      this->id = type;
    }
    message(message_category category, uint16_t type)
        : category(category), id(type) {}
    message(const uint16_t cat, const uint16_t id, const std::span<const uint8_t> msg_data)
        : category(cat), id(id), data(msg_data.begin(), msg_data.end()) {}
  };

  using message_channel = channel<message>;

}  // namespace other

#endif  // OTHER_CORE_MESSAGE_MESSAGE_DEFINES_HPP