/**
 * \file core/state_machine.hpp
 **/
#ifndef OTHER_CORE_CORE_STATE_MACHINE_HPP
#define OTHER_CORE_CORE_STATE_MACHINE_HPP

#include <algorithm>
#include <cstddef>
#include <ranges>
#include <unordered_map>

#include "core/logger.hpp"

namespace other {

  template <typename ST, typename ET>
    requires std::is_enum_v<ST> && std::is_enum_v<ET> &&
    requires(ST s, ET e) {
      ST::NUM_STATES;
      ET::NUM_EVENTS;
    }
  struct state_machine {
    state_machine(ST start_state)
        : current_state(start_state) {}
    ~state_machine() = default;

   protected:
    using action = std::function<void(ST, ET, ST, void*)>;

    struct transition {
      ST from;
      ET event;
      ST to;
      action on_transition;

      constexpr auto operator<=>(const transition& other) const {
        return std::tie(from, event) <=> std::tie(other.from, other.event);
      }
    };

   public:
    ST get_current_state() const { return current_state; }

    virtual void handle_event(ET event, void* data) {
      ST current = get_current_state();
      OTHER_ASSERT(current < ST::NUM_STATES, "Current state is invalid");

      auto& transitions = transition_table[static_cast<size_t>(current)];
      auto itr = std::ranges::find_if(transitions, [event](const transition& t) { return t.event == event; });
      if (itr != transitions.end()) {
        ST next_state = itr->to;
        OTHER_ASSERT(next_state < ST::NUM_STATES, "Next state is invalid");

        on_exit_state(current);
        if (itr->on_transition) {
          itr->on_transition(current, event, next_state, data);
        }
        on_enter_state(next_state);
        current_state = next_state;
        return;
      }

      CORE_LOG_WARN("No valid transition for event '{}' in state '{}'", static_cast<int>(event), static_cast<int>(get_current_state()));
    }

    virtual void on_enter_state(ST state) {}
    virtual void on_exit_state(ST state) {}

   protected:
    std::array<std::vector<transition>, static_cast<size_t>(ST::NUM_STATES)> transition_table = {};

    void add_transition(ST from, ET event, ST to, action on_transition = nullptr) {
      OTHER_ASSERT(from < ST::NUM_STATES, "Invalid 'from' state");
      OTHER_ASSERT(event < ET::NUM_EVENTS, "Invalid 'event'");

      auto itr = std::ranges::find_if(transition_table[static_cast<size_t>(from)], [event](const transition& t) { return t.event == event; });
      OTHER_ASSERT(itr == transition_table[static_cast<size_t>(from)].end(), "Transition already exists");

      transition_table[static_cast<size_t>(from)].push_back({ from, event, to, on_transition });
    }

   private:
    ST current_state;

    std::unordered_map<ST, std::unordered_map<ET, ST>> transitions;
  };

}  // namespace other

#endif  // OTHER_CORE_CORE_STATE_MACHINE_HPP