/**
 * \file driver/acknowledgement_list.hpp
 **/
#ifndef OTHERLIB_DRIVER_ACKNOWLEDGEMENT_LIST_HPP
#define OTHERLIB_DRIVER_ACKNOWLEDGEMENT_LIST_HPP

#include "thread/messages.hpp"

#include "network/message.hpp"
#include "network/message_handler.hpp"

namespace other {

  struct acknowledgement_list {
    struct pending_ack {
      natural_t id = 0;

      message_header header;
      microseconds timeout_duration = microseconds(0);
      std::chrono::time_point<std::chrono::steady_clock> sent_time;

      message_handler handler;

      asio::steady_timer timer;

      constexpr auto operator<=>(const pending_ack& other) const {
        return sent_time.time_since_epoch() <=> other.sent_time.time_since_epoch();
      }
    };
    natural_t next_pending_ack_id = 1;
    std::deque<pending_ack> pending_acks;
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_ACKNOWLEDGEMENT_LIST_HPP