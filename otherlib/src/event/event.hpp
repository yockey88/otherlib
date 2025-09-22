/**
 * \file event/event.hpp
 **/
#ifndef OTHER_EVENT_EVENT_HPP
#define OTHER_EVENT_EVENT_HPP

namespace other {

  enum class event_type {
    WINDOW_CLOSE_REQUESTED,
    WINDOW_MINIMIZE_REQUESTED,
    WINDOW_MAXIMIZE_REQUESTED,
  };

  struct event {
    event_type type;
  };

}  // namespace other

#endif  // OTHER_EVENT_EVENT_HPP