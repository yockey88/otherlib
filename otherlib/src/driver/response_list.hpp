/**
 * \file driver/response_list.hpp
 **/
#ifndef OTHERLIB_DRIVER_RESPONSE_LIST_HPP
#define OTHERLIB_DRIVER_RESPONSE_LIST_HPP

#include "network/message.hpp"
#include "network/message_handler.hpp"

namespace other {

  struct response_list {
    struct pending_response {
      natural_t id = 0;

      message_header header;
      std::chrono::time_point<std::chrono::steady_clock> sent_time;

      message_handler handler;

      asio::steady_timer timer;

      constexpr auto operator<=>(const pending_response& other) const {
        return header <=> other.header;
      }
    };
    natural_t next_pending_response_id = 1;
    std::deque<pending_response> pending_responses;
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_RESPONSE_LIST_HPP