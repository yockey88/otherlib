/**
 * \file event/event_system.cpp
 **/
#include "event/event_system.hpp"

#include <algorithm>
#include <deque>

#include <asio/asio.hpp>

#include "core/fnv.hpp"
#include "core/logger.hpp"

#include "event.hpp"

namespace other {

  struct event_system::timer_storage {
    struct event_timer {
      natural_t event_id;
      asio::steady_timer timer;
    };
    std::deque<event_timer> entries;
  };

  event_system::event_system(asio::io_context& io_ctx)
      : io_context(io_ctx), timers(make_scope<timer_storage>()) {}

  event_system::~event_system() = default;

  natural_t event_system::active_timer_count() const {
    std::scoped_lock lock(events_mutex);
    return timers->entries.size();
  }

  void event_system::clear() {
    cancel_all();
    {
      std::scoped_lock lock(events_mutex);
      registered_events.clear();
      timers->entries.clear();
    }
    CORE_LOG_INFO("Cleared all events and listeners");
  }

  /**
   * \note IMPORTANT: Do not use the logger in event_system::trigger_event
   *                  there are log sinks that need to trigger events and loggers are not re-entrant
   *                  so the logger can deadlock if the event system attempts to log during event triggering.
   *                  This is not a problem for event handlers that are not the specific event handlers used by log sinks
   * \todo add an 'event log' so that we can produce a history of registered/triggered events without risking deadlock
   *       it would also be nice so that we can expose an event system scene component or something of the like and users
   *       could use it to debug events
   **/

  void event_system::trigger_event(const std::string_view name) {
    natural_t event_id = FNV(name);
    trigger_event(event_id);
  }

  void event_system::trigger_event(natural_t event_id) {
    ostd::vector<event::handler> listeners;
    value data;
    {
      std::scoped_lock lock(events_mutex);
      auto itr = std::find_if(registered_events.begin(), registered_events.end(), [event_id](const event_ctx& ctx) {
        return ctx.ev.id == event_id;
      });
      if (itr == registered_events.end()) {
        return;
      }

      listeners = itr->listeners;
      data = itr->ev.data;
    }

    for (const auto& listener : listeners) {
      if (listener) {
        listener(data);
      }
    }
  }

  natural_t event_system::register_timed_event(const std::string_view name, microseconds duration, bool recurring) {
    natural_t id = FNV(name);

    {
      std::scoped_lock lock(events_mutex);
      auto itr = std::find_if(registered_events.begin(), registered_events.end(), [id](const event_ctx& ctx) {
        return ctx.ev.id == id;
      });
      if (itr != registered_events.end()) {
        return id;
      }
    }

    event ev{
      .id = id,
      .name = std::string(name),
      .duration = duration,
      .recurring = recurring,
    };
    {
      std::scoped_lock lock(events_mutex);
      registered_events.push_back({ ev, {} });
    }
    post_event_callback(id, duration);

    return id;
  }

  natural_t event_system::register_event(const std::string_view name) {
    natural_t id = FNV(name);

    {
      std::scoped_lock lock(events_mutex);
      auto itr = std::find_if(registered_events.begin(), registered_events.end(), [id](const event_ctx& ctx) {
        return ctx.ev.id == id;
      });
      if (itr != registered_events.end()) {
        return id;
      }
    }

    event ev{
      .id = id,
      .name = std::string(name),
      .duration = microseconds::zero(),
      .recurring = false,
    };
    {
      std::scoped_lock lock(events_mutex);
      registered_events.push_back({ ev, {} });
    }

    CORE_LOG_DEBUG("Registered event {} with ID {}", name, id);
    return id;
  }

  void event_system::cancel_event(const std::string_view name) {
    natural_t id = FNV(name);
    cancel_event(id);
  }

  void event_system::cancel_event(natural_t event_id) {
    std::scoped_lock lock(events_mutex);
    auto itr = std::find_if(registered_events.begin(), registered_events.end(), [event_id](const event_ctx& ctx) {
      return ctx.ev.id == event_id;
    });
    if (itr == registered_events.end()) {
      /// expected if event system is cleared before the timer is polled to call the final cancel,
      ///  usually will occur if clear is called before the events are fully purged
      CORE_LOG_TRACE("Attempted to cancel unregistered event ID {}", event_id);
      return;
    }
    itr->listeners.clear();
    itr->ev.data = value{};
    registered_events.erase(itr);

    auto timer_itr = std::find_if(timers->entries.begin(), timers->entries.end(), [event_id](const timer_storage::event_timer& et) {
      return et.event_id == event_id;
    });
    if (timer_itr != timers->entries.end()) {
      timer_itr->timer.cancel();
      timers->entries.erase(timer_itr);
    }

    CORE_LOG_DEBUG("Cancelled event with ID {}", event_id);
  }

  void event_system::set_user_data(natural_t event_id, const value& data) {
    std::scoped_lock lock(events_mutex);
    auto itr = std::find_if(registered_events.begin(), registered_events.end(), [event_id](const event_ctx& ctx) { return ctx.ev.id == event_id; });
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
    std::scoped_lock lock(events_mutex);
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
    std::scoped_lock lock(events_mutex);
    for (auto& timer : timers->entries) {
      timer.timer.cancel();
    }
  }

  bool event_system::has_event(const std::string_view name) const {
    return has_event(FNV(name));
  }

  bool event_system::has_event(natural_t event_id) const {
    std::scoped_lock lock(events_mutex);
    return std::ranges::find_if(registered_events, [event_id](const event_ctx& ctx) { return ctx.ev.id == event_id; }) != registered_events.end();
  }

  void event_system::post_event_callback(natural_t event_id, microseconds duration) {
    {
      std::scoped_lock lock(events_mutex);
      /// one timer entry per event: recurring events re-arm their existing timer instead of
      ///  appending a new (never-erased) entry every period
      auto timer_itr = std::find_if(timers->entries.begin(), timers->entries.end(), [event_id](const timer_storage::event_timer& et) {
        return et.event_id == event_id;
      });
      if (timer_itr == timers->entries.end()) {
        timers->entries.emplace_back(timer_storage::event_timer{ event_id, asio::steady_timer(io_context) });
        timer_itr = std::prev(timers->entries.end());
      }

      auto& timer = *timer_itr;
      timer.timer.expires_after(duration);
      timer.timer.async_wait([this, event_id](const asio::error_code& ec) {
        if (ec && ec != asio::error::operation_aborted) {
          CORE_LOG_ERROR("Event {} timer error: {}", event_id, ec.message());
          return;
        } else if (ec) {
          if (has_event(event_id)) {
            cancel_event(event_id);
          }
          return;
        }

        if (!ec) {
          trigger_event(event_id);

          microseconds next_duration = microseconds::zero();
          bool recurring = false;
          {
            std::scoped_lock lock(events_mutex);
            auto event_itr = std::find_if(registered_events.begin(), registered_events.end(), [event_id](const event_ctx& ctx) { return ctx.ev.id == event_id; });
            if (event_itr == registered_events.end()) {
              /// this can happen if the event was cancelled in between the timer being set and the callback being invoked
              return;
            }
            recurring = event_itr->ev.recurring;
            next_duration = event_itr->ev.duration;
          }

          if (recurring) {
            post_event_callback(event_id, next_duration);
          } else {
            cancel_event(event_id);
          }
        }
      });
    }
  }

}  // namespace other