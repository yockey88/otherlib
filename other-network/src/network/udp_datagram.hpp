/**
 * \file network/udp_datagram.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_UDP_DATAGRAM_HPP
#define OTHER_NETWORK_NETWORK_UDP_DATAGRAM_HPP

#include <cstdint>

namespace other {
#pragma pack(push, 1)

  enum udp_packet_type : uint16_t {
    UDP_CHECK_IN = 1,
    SCENE_STATE_UPDATE,
  };

  struct udp_check_in {
    uint64_t hash;
    uint64_t timestamp;
  };

  struct scene_state_update {
  };

  struct udp_datagram {
    udp_packet_type type;
    union payload {
      udp_check_in check_in;
    } packet;
  };

#pragma pack(pop)
}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_UDP_DATAGRAM_HPP