/**
 * \file network/message.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_MESSAGE_HPP
#define OTHER_NETWORK_NETWORK_MESSAGE_HPP

namespace other {

  // #pragma pack(push, 1)
  //   struct message_header_tcp {
  //     message_header header;
  //     uint32_t packet_size = 0;
  //   };

  //   struct network_packet_header {
  //     uint32_t packet_size = 0;
  //     uint16_t packet_type = 0;
  //     uint16_t num_messages = 0;
  //   };
  // #pragma pack(pop)

  //   struct tcp_packet {
  //     constexpr static inline size_t kMaxSize = 1448 - sizeof(network_packet_header);

  //     network_packet_header net_header;
  //     std::vector<message> messages;
  //   };

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_MESSAGE_HPP