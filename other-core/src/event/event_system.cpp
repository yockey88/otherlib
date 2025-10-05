/**
 * \file event/event_system.cpp
 **/
#include "event/event_system.hpp"

#include "core/fnv.hpp"
#include "core/logger.hpp"

#include "event.hpp"

namespace other {

  void event_system::poll() {
    while (!pending_event_cancellations.empty()) {
      natural_t event_id = pending_event_cancellations.front();
      pending_event_cancellations.pop();

      auto itr = registered_events.find(event_id);
      if (itr != registered_events.end()) {
        registered_events.erase(itr);
        event_listeners.erase(event_id);
        CORE_LOG_DEBUG("Removed event with ID {}", event_id);
      } else {
        CORE_LOG_WARN("Attempted to cancel unregistered event ID {}", event_id);
      }
    }
  }

  natural_t event_system::register_event(const std::string_view name, microsecond duration, bool recurring) {
    natural_t id = FNV(name);
    auto itr = registered_events.find(id);
    if (itr != registered_events.end()) {
      CORE_LOG_WARN("Event '{}' already registered with ID {}", name, id);
      return 0;
    }

    event ev{
      .id = id,
      .name = std::string(name),
      .duration = duration,
      .recurring = recurring,
    };

    auto [inserted_itr, inserted] = registered_events.emplace(id, event_ctx{ .ev = ev, .timer = asio::steady_timer(io_context) });
    OTHER_ASSERT(inserted, "Failed to register event '{}'", name);
    CORE_LOG_INFO("Registered event '{}' with ID {}", name, id);

    event_listeners.emplace(id, std::vector<event::handler>{});
    post_event_callback(id, duration);

    return id;
  }

  void event_system::cancel_event(const std::string_view name) {
    natural_t id = FNV(name);
    cancel_event(id);
    CORE_LOG_INFO("Cancelled event '{}'", name);
  }

  void event_system::cancel_event(natural_t event_id) {
    auto itr = registered_events.find(event_id);
    if (itr == registered_events.end()) {
      CORE_LOG_ERROR("Attempted to cancel unregistered event ID {}", event_id);
      return;
    }

    itr->second.timer.cancel();
    CORE_LOG_INFO("Cancelled event with ID {}", event_id);
  }

  void event_system::set_user_data(natural_t event_id, const value& data) {
    auto itr = registered_events.find(event_id);
    if (itr == registered_events.end()) {
      CORE_LOG_ERROR("Attempted to set user data for unregistered event ID {}", event_id);
      return;
    }

    itr->second.ev.data = data;
  }

  void event_system::add_listener(const std::string_view name, event::handler callback) {
    natural_t id = FNV(name);
    CORE_LOG_INFO("Adding listener for event '{}'", name);
    add_listener(id, std::move(callback));
  }

  void event_system::add_listener(natural_t id, std::function<void(const value&)> callback) {
    auto itr = registered_events.find(id);
    if (itr == registered_events.end()) {
      CORE_LOG_ERROR("Attempted to add listener for unregistered event ID {}", id);
      return;
    }

    auto listener_itr = event_listeners.find(id);
    OTHER_ASSERT(listener_itr != event_listeners.end(), "Event listeners list not found for event ID {}", id);
    listener_itr->second.push_back(callback);
  }

  void event_system::cancel_all() {
    for (auto& [id, ctx] : registered_events) {
      ctx.timer.cancel();
    }
  }

  void event_system::queue_event_removal(natural_t event_id) {
    auto itr = registered_events.find(event_id);
    if (itr == registered_events.end()) {
      CORE_LOG_ERROR("Attempted to queue removal for unregistered event ID {}", event_id);
      return;
    }

    pending_event_cancellations.push(event_id);
    CORE_LOG_DEBUG("Queued removal of event with ID {}", event_id);
  }

  void event_system::post_event_callback(natural_t event_id, microsecond duration) {
    auto inserted_itr = registered_events.find(event_id);
    if (inserted_itr == registered_events.end()) {
      CORE_LOG_ERROR("Failed to find registered event with ID {}", event_id);
      return;
    }

    inserted_itr->second.timer.expires_after(duration);
    inserted_itr->second.timer.async_wait([this, event_id](const asio::error_code& ec) {
      if (ec && ec != asio::error::operation_aborted) {
        CORE_LOG_ERROR("Event {} timer error: {}", event_id, ec.message());
        return;
      } else if (ec) {
        queue_event_removal(event_id);
        return;
      }

      if (!ec) {
        auto event_itr = registered_events.find(event_id);
        if (event_itr == registered_events.end()) {
          CORE_LOG_ERROR("Failed to find event for callback!");
          return;
        }

        CORE_LOG_DEBUG("Firing event '{}'", event_itr->second.ev.name);
        auto listeners_itr = event_listeners.find(event_id);
        if (listeners_itr != event_listeners.end()) {
          for (const auto& listener : listeners_itr->second) {
            if (listener) {
              listener(event_itr->second.ev.data);
            }
          }
        }

        if (event_itr->second.ev.recurring) {
          CORE_LOG_DEBUG(" - rescheduling recurring event '{}'", event_itr->second.ev.name);
          post_event_callback(event_id, event_itr->second.ev.duration);
        } else {
          CORE_LOG_DEBUG(" - one-shot event '{}' completed", event_itr->second.ev.name);
          queue_event_removal(event_id);
        }
      }
    });
  }

}  // namespace other