/**
 * \file network/connection_route.hpp
 **/
#ifndef OTHER_NETWORK_CONNECTION_ROUTE_HPP
#define OTHER_NETWORK_CONNECTION_ROUTE_HPP

namespace other {

  class socket_transport_provider;

  struct connection_route {
    socket_transport_provider* provider = nullptr;
    void* opaque_handle = nullptr;
  };

}  // namespace other

#endif  // OTHER_NETWORK_CONNECTION_ROUTE_HPP