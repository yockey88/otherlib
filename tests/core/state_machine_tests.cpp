/**
 * \file core/state_machine_tests.cpp
 **/
#include "core/state_machine_tests.hpp"

#include "core/state_machine.hpp"

namespace other {

  TEST_F(state_machine_test, basic_state_machine_transition) {
    enum class test_states : natural_t {
      STATE_A = 0,
      STATE_B,
      STATE_C,

      NUM_STATES,
    };
    enum class test_events : natural_t {
      EVENT_X = 0,
      EVENT_Y,

      NUM_EVENTS,
    };

    class test_state_machine : public state_machine<test_states, test_events> {
     public:
      test_state_machine()
          : state_machine<test_states, test_events>(test_states::STATE_A) {
        add_transition(test_states::STATE_A, test_events::EVENT_X, test_states::STATE_B);
        add_transition(test_states::STATE_B, test_events::EVENT_Y, test_states::STATE_C);
      }
      virtual ~test_state_machine() = default;
    };

    test_state_machine sm;

    EXPECT_EQ(sm.get_current_state(), test_states::STATE_A);

    sm.handle_event(test_events::EVENT_X, nullptr);
    EXPECT_EQ(sm.get_current_state(), test_states::STATE_B);

    sm.handle_event(test_events::EVENT_Y, nullptr);
    EXPECT_EQ(sm.get_current_state(), test_states::STATE_C);
  }

  TEST_F(state_machine_test, loop_state) {
    enum class test_states : natural_t {
      STATE_A = 0,
      STATE_B,
      STATE_C,

      NUM_STATES,
    };
    enum class test_events : natural_t {
      EVENT_X = 0,
      EVENT_Y,

      NUM_EVENTS,
    };

    class test_state_machine : public state_machine<test_states, test_events> {
     public:
      test_state_machine()
          : state_machine<test_states, test_events>(test_states::STATE_A) {
        add_transition(test_states::STATE_A, test_events::EVENT_X, test_states::STATE_B);
        add_transition(test_states::STATE_B, test_events::EVENT_Y, test_states::STATE_B);
        add_transition(test_states::STATE_B, test_events::EVENT_X, test_states::STATE_C);
        add_transition(test_states::STATE_C, test_events::EVENT_Y, test_states::STATE_A);
      }
      virtual ~test_state_machine() = default;
    };

    test_state_machine sm;

    EXPECT_EQ(sm.get_current_state(), test_states::STATE_A);

    sm.handle_event(test_events::EVENT_X, nullptr);
    EXPECT_EQ(sm.get_current_state(), test_states::STATE_B);

    sm.handle_event(test_events::EVENT_Y, nullptr);
    EXPECT_EQ(sm.get_current_state(), test_states::STATE_B);

    sm.handle_event(test_events::EVENT_X, nullptr);
    EXPECT_EQ(sm.get_current_state(), test_states::STATE_C);

    sm.handle_event(test_events::EVENT_Y, nullptr);
    EXPECT_EQ(sm.get_current_state(), test_states::STATE_A);
  }

}  // namespace other