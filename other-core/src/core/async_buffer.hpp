/**
 * \file core/async_buffer.hpp
 **/
#ifndef OTHER_CORE_CORE_ASYNC_BUFFER_HPP
#define OTHER_CORE_CORE_ASYNC_BUFFER_HPP

#include <array>
#include <deque>
#include <vector>

namespace other {

  template <size_t kBufferSize>
  struct async_buffer {
    bool reading = false;
    std::array<uint8_t, kBufferSize> read_buffer{};
    std::deque<std::vector<uint8_t>> read_queue{};

    bool writing = false;
    std::array<uint8_t, kBufferSize> write_buffer{};
    std::deque<std::vector<uint8_t>> write_queue{};
  };

}  // namespace other

#endif  // OTHER_CORE_CORE_ASYNC_BUFFER_HPP