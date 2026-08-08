/**
 * \file network/asio_endpoint.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_ASIO_ENDPOINT_HPP
#define OTHER_NETWORK_NETWORK_ASIO_ENDPOINT_HPP

#include <asio/asio.hpp>

#include "message/messages.hpp"

namespace other {

  /// inverse of binding_point::from_asio; lives module-side so other-core stays free
  ///  of concrete asio protocol types
  template <typename Protocol>
  typename Protocol::endpoint to_asio_endpoint(const binding_point& bp) {
    return typename Protocol::endpoint(asio::ip::address_v4(bp.ip), bp.port);
  }

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_ASIO_ENDPOINT_HPP