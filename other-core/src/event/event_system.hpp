/**
 * \file event/event_system.hpp
 **/
#ifndef OTHER_CORE_EVENT_EVENT_SYSTEM_HPP
#define OTHER_CORE_EVENT_EVENT_SYSTEM_HPP

#include <string_view>

#include <asio/asio.hpp>

#include "core/defines.hpp"
#include "core/fnv.hpp"
#include "core/timer.hpp"
#include "core/value.hpp"
#include "event/event.hpp"

namespace other {

  class event_system {
   public:
    event_system(asio::io_context& io_ctx)
        : io_context(io_ctx) {}
    virtual ~event_system() = default;

    void clear();

    void trigger_event(const std::string_view name);
    void trigger_event(natural_t event_id);

    template <typename T>
    void trigger_event(const std::string_view name, const T& data) {
      set_user_data(FNV(name), data);
      trigger_event(name);
    }

    natural_t register_timed_event(const std::string_view name, microseconds duration, bool recurring = false);
    natural_t register_event(const std::string_view name);

    void cancel_event(const std::string_view name);
    void cancel_event(natural_t event_id);

    void set_user_data(natural_t event_id, const value& data);

    template <typename T>
    void set_user_data(const std::string_view name, const T& data) {
      set_user_data(FNV(name), value{ data });
    }
    template <typename T>
    void set_user_data(natural_t event_id, const T& data) {
      set_user_data(event_id, value{ data });
    }

    void add_listener(const std::string_view name, event::handler callback);
    void add_listener(natural_t id, event::handler callback);

    void cancel_all();

   private:
    asio::io_context& io_context;

    struct event_ctx {
      event ev;
      std::vector<event::handler> listeners;
    };
    struct event_timer {
      natural_t event_id;
      asio::steady_timer timer;
    };
    std::vector<event_ctx> registered_events;
    std::vector<event_timer> event_timers;

    void post_event_callback(natural_t event_id, microseconds duration);
  };

}  // namespace other

#endif  // OTHER_CORE_EVENT_EVENT_SYSTEM_HPP