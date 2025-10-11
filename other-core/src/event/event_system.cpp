/**
 * \file event/event_system.cpp
 **/
#include "event/event_system.hpp"

#include <algorithm>

#include "core/fnv.hpp"
#include "core/logger.hpp"

#include "event.hpp"

namespace other {

  void event_system::clear() {
    cancel_all();
    registered_events.clear();
    event_timers.clear();
    CORE_LOG_INFO("Cleared all events and listeners");
  }

  void event_system::trigger_event(const std::string_view name) {
    natural_t event_id = FNV(name);
    trigger_event(event_id);
  }

  void event_system::trigger_event(natural_t event_id) {
    auto itr = std::find_if(registered_events.begin(), registered_events.end(), [event_id](const event_ctx& ctx) {
      return ctx.ev.id == event_id;
    });
    if (itr == registered_events.end()) {
      return;
    }

    for (const auto& listener : itr->listeners) {
      if (listener) {
        listener(itr->ev.data);
      }
    }
  }

  natural_t event_system::register_timed_event(const std::string_view name, microseconds duration, bool recurring) {
    natural_t id = FNV(name);

    event ev{
      .id = id,
      .name = std::string(name),
      .duration = duration,
      .recurring = recurring,
    };
    registered_events.push_back({ ev, {} });
    post_event_callback(id, duration);

    return id;
  }

  natural_t event_system::register_event(const std::string_view name) {
    natural_t id = FNV(name);

    event ev{
      .id = id,
      .name = std::string(name),
      .duration = microseconds::zero(),
      .recurring = false,
    };
    registered_events.push_back({ ev, {} });

    return id;
  }

  void event_system::cancel_event(const std::string_view name) {
    natural_t id = FNV(name);
    cancel_event(id);
    CORE_LOG_INFO("Cancelled event '{}'", name);
  }

  void event_system::cancel_event(natural_t event_id) {
    auto itr = std::find_if(registered_events.begin(), registered_events.end(), [event_id](const event_ctx& ctx) {
      return ctx.ev.id == event_id;
    });
    if (itr == registered_events.end()) {
      /// expected if event system is cleared before the timer is polled to call the final cancel,
      ///  usually will occur if clear is called before the events are fully purged
      return;
    }
    itr->listeners.clear();
    itr->ev.data = value{};
    registered_events.erase(itr);

    auto timer_itr = std::find_if(event_timers.begin(), event_timers.end(), [event_id](const event_timer& et) {
      return et.event_id == event_id;
    });
    if (timer_itr != event_timers.end()) {
      timer_itr->timer.cancel();
      event_timers.erase(timer_itr);
    }

    CORE_LOG_INFO("Cancelled event with ID {}", event_id);
  }

  void event_system::set_user_data(natural_t event_id, const value& data) {
    auto itr = std::find_if(registered_events.begin(), registered_events.end(), [event_id](const event_ctx& ctx) {
      return ctx.ev.id == event_id;
    });
    if (itr == registered_events.end()) {
      CORE_LOG_ERROR("Attempted to set user data for unregistered event ID {}", event_id);
      return;
    }

    itr->ev.data = data;
  }

  void event_system::add_listener(const std::string_view name, event::handler callback) {
    natural_t id = FNV(name);
    add_listener(id, std::move(callback));
  }

  void event_system::add_listener(natural_t id, event::handler callback) {
    auto itr = std::find_if(registered_events.begin(), registered_events.end(), [id](const event_ctx& ctx) {
      return ctx.ev.id == id;
    });
    if (itr == registered_events.end()) {
      CORE_LOG_ERROR("Attempted to add listener for unregistered event ID {}", id);
      return;
    }

    itr->listeners.push_back(std::move(callback));
  }

  void event_system::cancel_all() {
    for (auto& timer : event_timers) {
      timer.timer.cancel();
    }
  }

  void event_system::post_event_callback(natural_t event_id, microseconds duration) {
    auto& timer = event_timers.emplace_back(event_timer{ event_id, asio::steady_timer(io_context) });
    timer.timer.expires_after(duration);
    timer.timer.async_wait([this, event_id](const asio::error_code& ec) {
      if (ec && ec != asio::error::operation_aborted) {
        CORE_LOG_ERROR("Event {} timer error: {}", event_id, ec.message());
        return;
      } else if (ec) {
        cancel_event(event_id);
        return;
      }

      if (!ec) {
        trigger_event(event_id);

        auto event_itr = std::find_if(registered_events.begin(), registered_events.end(), [event_id](const event_ctx& ctx) { return ctx.ev.id == event_id; });
        if (event_itr == registered_events.end()) {
          /// this can happen if the event was cancelled in between the timer being set and the callback being invoked
          return;
        }

        if (event_itr->ev.recurring) {
          post_event_callback(event_id, event_itr->ev.duration);
        } else {
          cancel_event(event_id);
        }
      }
    });
  }

}  // namespace other