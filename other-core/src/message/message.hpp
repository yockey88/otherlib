/**
 * \file message/message.hpp
 **/
#ifndef OTHER_CORE_MESSAGE_MESSAGE_HPP
#define OTHER_CORE_MESSAGE_MESSAGE_HPP

// clang-format off
#include "message/message_defines.hpp"
#include "message/message_fields.hpp"
#include "message/message_serialization.hpp"
#include "message/messages.hpp"
// clang-format on

namespace other {

  struct message_handler {
    using handler_fn = std::function<void(message_header, const std::span<const uint8_t>)>;
    using timeout_fn = std::function<void(message_header)>;
    handler_fn handle_msg = nullptr;
    timeout_fn on_timeout = nullptr;
  };

}  // namespace other

#endif  // OTHER_CORE_MESSAGE_MESSAGE_HPP