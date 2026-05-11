/**
 * \file test/network/network_thread_tests.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_THREAD_TESTS_HPP
#define OTHER_NETWORK_NETWORK_THREAD_TESTS_HPP

#include "other_test.hpp"

namespace other {

  class network_thread_tests : public other_test {
   protected:
    bool wait_on_message(const message_header& header, message_bus& bus, microseconds timeout = microseconds(100));
    bool wait_on_acked_message(const message_header& original_header, message_bus& bus, microseconds timeout = microseconds(100));
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_THREAD_TESTS_HPP