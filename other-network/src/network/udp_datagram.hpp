/**
 * \file network/udp_datagram.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_UDP_DATAGRAM_HPP
#define OTHER_NETWORK_NETWORK_UDP_DATAGRAM_HPP

#include <cstdint>

#include "core/defines.hpp"

namespace other {
#pragma pack(push, 1)

  enum udp_packet_type : uint16_t {
    UDP_CHECK_IN = 1,
    SCENE_ADD_ENTITY,
  };

  struct udp_check_in {
    natural_t hash;
    natural_t timestamp;
  };

  struct scene_add_entity {
    constexpr static inline uint16_t kMaxNameLength = 256;
    natural_t entity_id;
    uint16_t name_length;
    char name[kMaxNameLength];
  };

  struct udp_datagram {
    udp_packet_type type;
    union payload {
      udp_check_in check_in;
      scene_add_entity add_entity;
    } packet;
  };

#pragma pack(pop)
}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_UDP_DATAGRAM_HPP