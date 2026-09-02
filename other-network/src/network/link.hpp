/**
 * \file network/link.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_LINK_HPP
#define OTHER_NETWORK_NETWORK_LINK_HPP

#include "core/defines.hpp"
#include "core/time.hpp"

#include "network/node_id.hpp"

namespace other {

  enum class link_state : uint8_t {
    CONNECTING = 0,
    HANDSHAKING = 1,
    AUTHENTICATING = 2,
    UP = 3,
    DISCONNECTING = 4,
    DOWN = 5,
  };

  enum class link_close_reason : uint16_t {
    NONE = 0,
    SHUTDOWN = 1,
    PROTOCOL_ERROR = 2,
    HANDSHAKE_TIMEOUT = 3,
    KEEPALIVE_TIMEOUT = 4,
    SECURITY_ERROR = 5,
    TRANSPORT_CLOSED = 6,
    ACTOR_DESTROYED = 7,
    REMOTE_BYE = 8,
  };

  struct link_caps {
    bool reliable = true;
    bool ordered = true;
    uint32_t max_frame_size = 0;
  };

  struct link_record {
    natural_t link_id = 0;
    node_id local = 0;
    node_id remote = 0;

    natural_t connection_id = 0;

    link_state state = link_state::DOWN;
    link_caps caps;

    bool attested = false;

    microseconds rtt{ 0 };

    natural_t latest_tx_tick = 0;
    natural_t latest_rx_tick = 0;

    struct {
      natural_t frames_tx = 0;
      natural_t frames_rx = 0;
      natural_t bytes_tx = 0;
      natural_t bytes_rx = 0;
    } stats{};
  };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_LINK_HPP