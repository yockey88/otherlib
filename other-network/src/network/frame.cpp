/**
 * \file network/frame.cpp
 **/
#include "network/frame.hpp"

#include <bit>

static_assert(std::endian::native == std::endian::little, "frame codec requires a little-endian target");

namespace other {

  namespace {

    inline void put_u16(uint8_t* out, uint16_t v) {
      out[0] = static_cast<uint8_t>(v);
      out[1] = static_cast<uint8_t>(v >> 8);
    }

    inline void put_u32(uint8_t* out, uint32_t v) {
      out[0] = static_cast<uint8_t>(v);
      out[1] = static_cast<uint8_t>(v >> 8);
      out[2] = static_cast<uint8_t>(v >> 16);
      out[3] = static_cast<uint8_t>(v >> 24);
    }

    inline void put_u64(uint8_t* out, uint64_t v) {
      put_u32(out, static_cast<uint32_t>(v));
      put_u32(out + 4, static_cast<uint32_t>(v >> 32));
    }

    inline uint16_t get_u16(const uint8_t* in) {
      return static_cast<uint16_t>(in[0]) | (static_cast<uint16_t>(in[1]) << 8);
    }

    inline uint32_t get_u32(const uint8_t* in) {
      return static_cast<uint32_t>(in[0]) | (static_cast<uint32_t>(in[1]) << 8) |
             (static_cast<uint32_t>(in[2]) << 16) | (static_cast<uint32_t>(in[3]) << 24);
    }

    inline uint64_t get_u64(const uint8_t* in) {
      return static_cast<uint64_t>(get_u32(in)) | (static_cast<uint64_t>(get_u32(in + 4)) << 32);
    }

  }  // namespace

  void write_frame_header(const frame_header& header, uint8_t* out) {
    put_u32(out, header.length);
    put_u16(out + 4, header.net_id);
    put_u16(out + 6, header.flags);
  }

  frame_header read_frame_header(std::span<const uint8_t> bytes) {
    OTHER_ASSERT(bytes.size() >= kFrameHeaderSize, "read_frame_header needs {} bytes, given {}", kFrameHeaderSize, bytes.size());
    frame_header header;
    header.length = get_u32(bytes.data());
    header.net_id = get_u16(bytes.data() + 4);
    header.flags = get_u16(bytes.data() + 6);
    return header;
  }

  void write_route_header(const route_header& route, uint8_t* out) {
    put_u64(out, route.src);
    put_u64(out + 8, route.dst);
    out[16] = route.ttl;
  }

  opt<route_header> read_route_header(std::span<const uint8_t> payload) {
    if (payload.size() < kRouteHeaderSize) {
      return std::nullopt;
    }
    route_header route;
    route.src = get_u64(payload.data());
    route.dst = get_u64(payload.data() + 8);
    route.ttl = payload[16];
    return route;
  }

  ostd::vector<uint8_t> write_frame(uint16_t net_id, std::span<const uint8_t> payload) {
    ostd::vector<uint8_t> frame(kFrameHeaderSize + payload.size());
    const frame_header header{
      .length = static_cast<uint32_t>(kFrameLengthFloor + payload.size()),
      .net_id = net_id,
      .flags = static_cast<uint16_t>(frame_flags::NONE),
    };
    write_frame_header(header, frame.data());
    std::ranges::copy(payload, frame.data() + kFrameHeaderSize);
    return frame;
  }

  ostd::vector<uint8_t> write_flagged_frame(uint16_t net_id, uint16_t flags, std::span<const uint8_t> payload) {
    ostd::vector<uint8_t> frame(kFrameHeaderSize + payload.size());
    const frame_header header{
      .length = static_cast<uint32_t>(kFrameLengthFloor + payload.size()),
      .net_id = net_id,
      .flags = flags,
    };
    write_frame_header(header, frame.data());
    std::ranges::copy(payload, frame.data() + kFrameHeaderSize);
    return frame;
  }

  ostd::vector<uint8_t> write_routed_frame(const route_header& route, uint16_t net_id, std::span<const uint8_t> payload) {
    ostd::vector<uint8_t> frame(kFrameHeaderSize + kRouteHeaderSize + payload.size());
    const frame_header header{
      .length = static_cast<uint32_t>(kFrameLengthFloor + kRouteHeaderSize + payload.size()),
      .net_id = net_id,
      .flags = static_cast<uint16_t>(frame_flags::ROUTED),
    };
    write_frame_header(header, frame.data());
    write_route_header(route, frame.data() + kFrameHeaderSize);
    std::ranges::copy(payload, frame.data() + kFrameHeaderSize + kRouteHeaderSize);
    return frame;
  }

  void frame_reader::feed(std::span<const uint8_t> chunk) {
    if (poisoned) {
      return;
    }
    pending.insert(pending.end(), chunk.begin(), chunk.end());
  }

  void frame_reader::compact() {
    if (consumed == 0) {
      return;
    }
    if (consumed >= pending.size()) {
      pending.clear();
    } else if (consumed > buffered()) {
      pending.erase(pending.begin(), pending.begin() + static_cast<ptrdiff_t>(consumed));
    } else {
      return;
    }
    consumed = 0;
  }

  frame_parse_result frame_reader::next() {
    if (poisoned) {
      return { .err = frame_parse_result::error::MALFORMED };
    }
    if (buffered() < kFrameHeaderSize) {
      compact();
      return { .err = frame_parse_result::error::NEED_MORE };
    }

    const std::span<const uint8_t> view{ pending.data() + consumed, buffered() };
    const frame_header header = read_frame_header(view);
    if (header.length < kFrameLengthFloor) {
      poisoned = true;
      return { .err = frame_parse_result::error::MALFORMED };
    }
    if (header.length > max_frame_size) {
      poisoned = true;
      return { .err = frame_parse_result::error::OVERSIZE };
    }

    const size_t total = sizeof(uint32_t) + header.length;
    if (view.size() < total) {
      compact();
      return { .err = frame_parse_result::error::NEED_MORE };
    }

    parsed_frame frame{
      .net_id = header.net_id,
      .flags = header.flags,
      .payload = ostd::vector<uint8_t>(view.data() + kFrameHeaderSize, view.data() + total),
    };
    consumed += total;
    compact();
    return { .frame = std::move(frame) };
  }

  frame_parse_result parse_frame_exact(std::span<const uint8_t> bytes, uint32_t max_frame_size) {
    if (bytes.size() < kFrameHeaderSize) {
      return { .err = frame_parse_result::error::MALFORMED };
    }

    const frame_header header = read_frame_header(bytes);
    if (header.length < kFrameLengthFloor) {
      return { .err = frame_parse_result::error::MALFORMED };
    }
    if (header.length > max_frame_size) {
      return { .err = frame_parse_result::error::OVERSIZE };
    }
    if (sizeof(uint32_t) + header.length != bytes.size()) {
      return { .err = frame_parse_result::error::MALFORMED };
    }

    return {
      .frame = parsed_frame{
        .net_id = header.net_id,
        .flags = header.flags,
        .payload = ostd::vector<uint8_t>(bytes.begin() + kFrameHeaderSize, bytes.end()),
      },
    };
  }

}  // namespace other
