/**
 * \file peer/peer_mailbox.hpp
 **/
#ifndef OTHER_NETWORK_PEER_PEER_MAILBOX_HPP
#define OTHER_NETWORK_PEER_PEER_MAILBOX_HPP

#include <vector>

#include "data-structures/spsc_buffer.hpp"

namespace other {

  // ~1s 60Hz packets per peer with headroom
  constexpr static size_t kPeerMailboxCapacity = 256;

  using peer_mailbox = spsc_buffer<std::vector<uint8_t>, kPeerMailboxCapacity>;

}  // namespace other

#endif  // OTHER_NETWORK_PEER_PEER_MAILBOX_HPP