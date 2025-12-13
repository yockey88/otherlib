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
    using handler_fn = void (driver::*)(message_header header, const std::span<const uint8_t>);
    using timeout_fn = void (driver::*)(message_header);
    driver* driver_ptr = nullptr;
    handler_fn handle_msg = nullptr;
    timeout_fn on_timeout = nullptr;

    message_handler(driver* driver_ptr)
        : driver_ptr(driver_ptr) {}
    message_handler(driver* driver_ptr, handler_fn on_acknowledgement, timeout_fn on_timeout)
        : driver_ptr(driver_ptr), handle_msg(on_acknowledgement), on_timeout(on_timeout) {}
    ~message_handler() = default;
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_MESSAGE_HANDLER_HPP