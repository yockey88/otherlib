/**
 * \file network/directory.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_DIRECTORY_HPP
#define OTHER_NETWORK_NETWORK_DIRECTORY_HPP

namespace other {

  class directory {
    /*
    struct connection {
      natural_t connection_number = 0;
      integer_t session_id = 0;
      binding_point endpoint;
      scope<session> active_session = nullptr;
    };
    std::deque<connection> pending_connections;
    std::unordered_map<integer_t, connection> client_endpoints;

    using event_callback = std::function<void(integer_t)>;
    std::unordered_map<integer_t, event_callback> check_in_listeners;

    struct udp_binding {
      natural_t connection_number = 0;
      integer_t udp_binding_id = 0;
      binding_point endpoint;

      scope<udp_stream> stream = nullptr;
      // udp_stream* stream = nullptr;
      // std::mutex* mutex = nullptr;

      // udp_handle handle;
    };
    integer_t next_udp_binding_id = 1;
    std::map<natural_t, udp_binding> udp_bindings;
    */
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_DIRECTORY_HPP