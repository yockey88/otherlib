/**
 * \file network/message_handler.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_MESSAGE_HANDLER_HPP
#define OTHER_NETWORK_NETWORK_MESSAGE_HANDLER_HPP

#include "thread/message.hpp"

#include "network/message.hpp"

namespace other {

  class driver;

  struct message_handler {
    using handler_fn = std::function<void(message_header, const std::span<const uint8_t>)>;
    using timeout_fn = std::function<void(message_header)>;
    handler_fn handle_msg = nullptr;
    timeout_fn on_timeout = nullptr;
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_MESSAGE_HANDLER_HPP