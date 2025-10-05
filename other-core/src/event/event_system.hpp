/**
 * \file event/event_system.hpp
 **/
#ifndef OTHER_CORE_EVENT_EVENT_SYSTEM_HPP
#define OTHER_CORE_EVENT_EVENT_SYSTEM_HPP

#include <queue>
#include <string_view>
#include <unordered_map>

#include <asio/asio.hpp>

#include "core/defines.hpp"
#include "core/timer.hpp"
#include "core/value.hpp"
#include "event/event.hpp"

namespace other {

  class event_system {
   public:
    event_system(asio::io_context& io_ctx)
        : io_context(io_ctx) {}
    virtual ~event_system() = default;

    void poll();

    natural_t register_event(const std::string_view name, microsecond duration, bool recurring = false);
    void cancel_event(const std::string_view name);
    void cancel_event(natural_t event_id);

    void set_user_data(natural_t event_id, const value& data);

    template <typename T>
    void set_user_data(natural_t event_id, const T& data) {
      set_user_data(event_id, value{ data });
    }

    void add_listener(const std::string_view name, std::function<void(const value&)> callback);
    void add_listener(natural_t id, std::function<void(const value&)> callback);

    void cancel_all();

   private:
    asio::io_context& io_context;

    struct event_ctx {
      event ev;
      asio::steady_timer timer;
    };
    std::unordered_map<natural_t, event_ctx> registered_events;
    std::unordered_map<natural_t, std::vector<std::function<void(const value&)>>> event_listeners;

    std::queue<natural_t> pending_event_cancellations;

    void queue_event_removal(natural_t event_id);

    void post_event_callback(natural_t event_id, microsecond duration);
  };

}  // namespace other

#endif  // OTHER_CORE_EVENT_EVENT_SYSTEM_HPP