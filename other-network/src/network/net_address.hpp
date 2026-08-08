/**
 * \file network/net_address.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_NET_ADDRESS_HPP
#define OTHER_NETWORK_NETWORK_NET_ADDRESS_HPP

#include "core/defines.hpp"

#include "message/messages.hpp"

namespace other {

  /// kind-tagged endpoint address; routes to a transport by kind unless the caller
  ///  names one explicitly (STEAM_* kinds stay reserved until the steam module lands)
  struct net_address {
    enum class kind : uint8_t {
      IP = 0,
      MEMORY = 1,
      STEAM_PEER = 2,
      STEAM_LOBBY = 3,
    };

    kind addressing = kind::IP;
    binding_point ip{};
    uint64_t id = 0;  // memory endpoint / steam id / lobby id

    static net_address ip_endpoint(const binding_point& bp) {
      return { .addressing = kind::IP, .ip = bp };
    }
    static net_address memory_endpoint(uint64_t endpoint_id) {
      return { .addressing = kind::MEMORY, .id = endpoint_id };
    }
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_NET_ADDRESS_HPP
