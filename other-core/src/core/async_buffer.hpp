/**
 * \file core/async_buffer.hpp
 **/
#ifndef OTHER_CORE_CORE_ASYNC_BUFFER_HPP
#define OTHER_CORE_CORE_ASYNC_BUFFER_HPP

#include <array>
#include <cstddef>
#include <deque>
#include <span>
#include <vector>

#include <asio/asio.hpp>

#include "core/logger.hpp"

namespace other {

  template <size_t kBufferSize>
  struct async_buffer {
    inline void start_read() { reading = true; }
    inline bool is_reading() const { return reading; }
    inline bool has_pending_read_data() const { return !read_queue.empty(); }
    std::vector<uint8_t> read() {
      if (read_queue.empty()) {
        return {};
      }
      auto data = std::move(read_queue.front());
      read_queue.pop_front();
      return data;
    }
    void finish_read(size_t bytes) {
      reading = false;
      read_queue.emplace_back(read_buffer.data(), read_buffer.data() + bytes);
      std::ranges::fill(read_buffer, 0);
    }

    inline void write(const std::span<uint8_t> data) { std::ranges::copy(data, write_buffer); }
    inline void buffer_write(const std::span<const uint8_t> data) { write_queue.emplace_back(std::vector(data.begin(), data.end())); }
    inline bool is_writing() const { return writing; }
    inline bool has_pending_write_data() const { return !write_queue.empty(); }
    std::vector<uint8_t> pending_write_data() {
      if (write_queue.empty()) {
        return {};
      }
      auto data = std::move(write_queue.front());
      write_queue.pop_front();
      return data;
    }
    void start_write() {
      writing = true;
      auto write_data = pending_write_data();
      OTHER_ASSERT(write_data.size() <= kBufferSize, "Pending write data size {} exceeds async buffer capacity of {}", write_data.size(), kBufferSize);
      std::ranges::copy(write_data, write_buffer.begin());
    }
    void finish_write() {
      writing = false;
      std::ranges::fill(write_buffer, 0);
    }

    auto asio_read_buffer() {
      return asio::buffer(read_buffer);
    }
    auto asio_write_buffer() const {
      return asio::buffer(write_buffer);
    }

   private:
    /// for writing/reading straight from/to socket/resource
    bool reading = false;
    std::array<uint8_t, kBufferSize> read_buffer{};
    std::deque<std::vector<uint8_t>> read_queue{};

    bool writing = false;
    std::array<uint8_t, kBufferSize> write_buffer{};
    std::deque<std::vector<uint8_t>> write_queue{};
  };

}  // namespace other

#endif  // OTHER_CORE_CORE_ASYNC_BUFFER_HPP