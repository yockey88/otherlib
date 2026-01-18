/**
 * \file driver/timer_list.hpp
 **/
#ifndef OTHERLIB_DRIVER_TIMER_LIST_HPP
#define OTHERLIB_DRIVER_TIMER_LIST_HPP

#include <deque>
#include <functional>

#include <asio/asio.hpp>

#include "core/defines.hpp"

namespace other {

  struct timer_list {
    struct timeout {
      using on_timeout = std::function<void(natural_t)>;

      natural_t id = 0;
      asio::steady_timer timer;
    };
    natural_t next_timeout_id = 1;
    std::deque<timeout> pending_timeouts;
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_TIMER_LIST_HPP