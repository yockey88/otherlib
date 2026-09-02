/**
 * \file network/net_address.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_NET_ADDRESS_HPP
#define OTHER_NETWORK_NETWORK_NET_ADDRESS_HPP

#include "message/messages.hpp"

namespace other {

  struct net_address {
    uint64_t transport_hash = 0;
    binding_point binding;
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_NET_ADDRESS_HPP