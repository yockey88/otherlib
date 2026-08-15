/**
 * \file network/listener_route.hpp
 **/
#ifndef OTHER_NETWORK_LISTENER_ROUTE_HPP
#define OTHER_NETWORK_LISTENER_ROUTE_HPP

namespace other {

  class socket_transport_provider;
  class packet_sink;

  struct listener_route {
    socket_transport_provider* provider = nullptr;
    void* opaque_handle = nullptr;
    packet_sink* sink = nullptr;
  };

}  // namespace other

#endif  // OTHER_NETWORK_LISTENER_ROUTE_HPP