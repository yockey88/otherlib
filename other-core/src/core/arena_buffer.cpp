/**
 * \file core/arena_buffer.cpp
 **/

#include "core/arena_buffer.hpp"

#include "core/logger.hpp"
#include "memory/arena.hpp"

namespace other {

  arena_buffer::arena_buffer(void* data, size_t size) {
    if (data == nullptr || size == 0) {
      throw std::runtime_error("Invalid arena buffer initialization: data is null or size is zero.");
    }

    allocate(size);
    write(data, size);
    element_sizes.push_back(size);
  }

  arena_buffer::~arena_buffer() {
    release();
  }

  arena_buffer::arena_buffer(arena_buffer&& other) {
    memory_start = other.memory_start;
    memory_cursor = other.memory_cursor;
    capacity = other.capacity;
    offset = other.offset;
    element_sizes = std::move(other.element_sizes);

    other.memory_start = nullptr;
    other.memory_cursor = nullptr;
    other.capacity = 0;
    other.offset = 0;
    other.element_sizes.clear();
  }

  arena_buffer::arena_buffer(const arena_buffer& other) {
    allocate(other.capacity);
    write(other.memory_start, other.capacity);
    offset = other.offset;
    element_sizes = other.element_sizes;
  }

  arena_buffer& arena_buffer::operator=(arena_buffer&& other) {
    if (this != &other) {
      memory_start = other.memory_start;
      memory_cursor = other.memory_cursor;
      capacity = other.capacity;
      offset = other.offset;
      element_sizes = std::move(other.element_sizes);

      other.memory_start = nullptr;
      other.memory_cursor = nullptr;
      other.capacity = 0;
      other.offset = 0;
      other.element_sizes.clear();
    }
    return *this;
  }

  arena_buffer& arena_buffer::operator=(const arena_buffer& other) {
    if (this != &other) {
      allocate(other.capacity);
      write(other.memory_start, other.capacity);
      offset = other.offset;
      element_sizes = other.element_sizes;
    }
    return *this;
  }

  std::string arena_buffer::dump_buffer() const {
    std::stringstream ss;
    ss << std::format("arena-buffer : [capacity: {} bytes, size: {} bytes, num_elements: {}] =\n[", capacity, offset, element_sizes.size());
    for (size_t i = 0; i < offset; ++i) {
      ss << std::format("{:#02x}", byte_at(i));
      if (i < offset - 1) {
        ss << " ";
      }
    }
    ss << "]";
    return ss.str();
  }

  void arena_buffer::allocate(uint64_t size) {
    release();
    if (size == 0) {
      return;
    }
    memory_start = subsystem<arena>::get()->allocate(size);
    memory_cursor = memory_start;
    capacity = size;
    OTHER_ASSERT(memory_start != nullptr, "Failed to allocate memory for arena buffer.");
  }

  void arena_buffer::extend() {
    OTHER_ASSERT(memory_start != nullptr && capacity > 0, "extend() called on an unallocated arena buffer.");
    const size_t new_capacity = capacity * 2;
    void* new_memory = subsystem<arena>::get()->allocate(new_capacity);
    OTHER_ASSERT(new_memory != nullptr, "Failed to extend arena buffer memory.");

    std::memcpy(new_memory, memory_start, offset);
    subsystem<arena>::get()->free(memory_start, capacity);

    memory_start = new_memory;
    memory_cursor = static_cast<uint8_t*>(new_memory) + offset;
    capacity = new_capacity;
  }

  void arena_buffer::release() {
    if (memory_start != nullptr) {
      subsystem<arena>::get()->free(memory_start, capacity);
      memory_start = nullptr;
      memory_cursor = nullptr;
      capacity = 0;
      offset = 0;
      element_sizes.clear();
    }
  }

  void arena_buffer::zero_mem() {
    if (memory_start != nullptr) {
      std::memset(memory_start, 0, capacity);
      memory_cursor = memory_start;
      offset = 0;
      element_sizes.clear();
    } else {
      OTHER_ASSERT(false, "Memory start is null, cannot zero memory in arena buffer.");
    }
  }

  void arena_buffer::zero_range(uint64_t offset, uint64_t size) {
    OTHER_ASSERT(memory_start != nullptr, "Memory start is null, cannot zero range in arena buffer.");
    OTHER_ASSERT(size > 0, "Size must be greater than zero for zeroing memory in arena buffer.");
    OTHER_ASSERT(offset + size <= capacity, "Offset and size exceed buffer capacity for zeroing memory in arena buffer.");

    std::memset(static_cast<uint8_t*>(memory_start) + offset, 0, size);
  }

  void arena_buffer::write(const void* data, size_t size) {
    if (size == 0) {
      return;  // Nothing to write
    }

    OTHER_ASSERT(data != nullptr, "Attempting to write null data to arena buffer.");
    OTHER_ASSERT(size <= capacity, "Attempting to write data larger than buffer capacity.");
    OTHER_ASSERT(offset + size <= capacity, "Attempting to write data that exceeds buffer capacity.");
    OTHER_ASSERT(memory_cursor != nullptr, "Memory cursor is null, cannot write to arena buffer.");
    OTHER_ASSERT(memory_start != nullptr, "Memory start is null, cannot write to arena buffer.");

    std::memcpy(static_cast<uint8_t*>(memory_cursor), data, size);
    memory_cursor = static_cast<uint8_t*>(memory_cursor) + size;
    offset += size;
  }

  void arena_buffer::shift_cursor(uint64_t size) {
    OTHER_ASSERT(memory_start != nullptr, "Attempting to shift cursor in uninitialized arena buffer");
    OTHER_ASSERT(size + offset <= capacity, "Attempting to shift cursor beyond buffer capacity");
    memory_cursor = static_cast<uint8_t*>(memory_cursor) + size;
    offset += size;
  }

  uint8_t& arena_buffer::byte_at(uint64_t offset) {
    return *static_cast<uint8_t*>(memory_at(offset));
  }

  const uint8_t& arena_buffer::byte_at(uint64_t offset) const {
    return *static_cast<const uint8_t*>(memory_at(offset));
  }

  void* arena_buffer::memory_at(uint64_t offset) {
    OTHER_ASSERT(memory_start != nullptr, "Attempting to access memory at offset {} in uninitialized arena buffer", offset);
    OTHER_ASSERT(offset < capacity, "Attempting to access memory at offset {} > capacity {}", offset, capacity);
    return static_cast<uint8_t*>(memory_start) + offset;
  }

  const void* arena_buffer::memory_at(uint64_t offset) const {
    OTHER_ASSERT(memory_start != nullptr, "Attempting to access memory at offset {} in uninitialized arena buffer", offset);
    OTHER_ASSERT(offset < capacity, "Attempting to access memory at offset {} > capacity {}", offset, capacity);
    return static_cast<const uint8_t*>(memory_start) + offset;
  }

}  // namespace other