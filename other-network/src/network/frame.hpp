/**
 * \file network/frame.hpp
 **/
#ifndef OTHER_NETWORK_NETWORK_FRAME_HPP
#define OTHER_NETWORK_NETWORK_FRAME_HPP

#include <span>

#include "core/defines.hpp"

#include "peer_mesh/node_id.hpp"

namespace other {

  enum class frame_flags : uint16_t {
    NONE = 0,
    ROUTED = 1 << 0,
    // reserved: compression, fragmentation, etc..
  };

  struct route_header {
    node_id src = 0;
    node_id dst = 0;
    uint8_t ttl = 0;
  };

  struct frame_header {
    /// bytes after the length field itself: 4 (net_id + flags) + payload size
    uint32_t length = 0;
    uint16_t net_id = 0;
    uint16_t flags = 0;
  };

  constexpr static size_t kFrameHeaderSize = 8;
  constexpr static size_t kRouteHeaderSize = 17;
  constexpr static uint32_t kFrameLengthFloor = 4;
  constexpr static uint32_t kDefaultMaxFrameSize = 2 * 1024 * 1024;

  /// fields are written/read individually, little-endian; never a struct memcpy
  void write_frame_header(const frame_header& header, uint8_t* out);
  frame_header read_frame_header(std::span<const uint8_t> bytes);

  void write_route_header(const route_header& route, uint8_t* out);
  opt<route_header> read_route_header(std::span<const uint8_t> payload);

  ostd::vector<uint8_t> write_frame(uint16_t net_id, std::span<const uint8_t> payload);
  ostd::vector<uint8_t> write_routed_frame(const route_header& route, uint16_t net_id, std::span<const uint8_t> payload);
  /// payload passed pre-composed (e.g. route header + ciphertext); flags verbatim
  ostd::vector<uint8_t> write_flagged_frame(uint16_t net_id, uint16_t flags, std::span<const uint8_t> payload);

  struct parsed_frame {
    uint16_t net_id = 0;
    uint16_t flags = 0;
    ostd::vector<uint8_t> payload;
  };

  struct frame_parse_result {
    enum class error : uint8_t {
      NONE = 0,
      NEED_MORE = 1,
      OVERSIZE = 2,
      MALFORMED = 3,
    };

    opt<parsed_frame> frame{};
    error err = error::NONE;
  };

  /// stream reassembly — owned by a network per stream-backed link; transports deliver bytes
  class frame_reader {
   public:
    explicit frame_reader(uint32_t max_frame_size = kDefaultMaxFrameSize)
        : max_frame_size(max_frame_size) {}

    void feed(std::span<const uint8_t> chunk);
    /// NEED_MORE until a whole frame is buffered; OVERSIZE/MALFORMED poison the stream
    frame_parse_result next();
    size_t buffered() const { return pending.size() - consumed; }

   private:
    ostd::vector<uint8_t> pending;
    size_t consumed = 0;
    uint32_t max_frame_size = kDefaultMaxFrameSize;
    bool poisoned = false;

    void compact();
  };

  /// message/datagram transports: one delivery must be exactly one frame
  frame_parse_result parse_frame_exact(std::span<const uint8_t> bytes, uint32_t max_frame_size = kDefaultMaxFrameSize);

}  // namespace other

#endif  // OTHER_NETWORK_NETWORK_FRAME_HPP
