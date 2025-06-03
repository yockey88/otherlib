/**
 * \file core/arena_buffer.hpp
 **/
#ifndef OTHER_CORE_ARENA_BUFFER_HPP
#define OTHER_CORE_ARENA_BUFFER_HPP

#include <numeric>

#include <glm/glm.hpp>

#include "core/defines.hpp"
#include "core/logger.hpp"

namespace other {

  class arena_buffer {
   public:
    arena_buffer() : memory_start(nullptr), memory_cursor(nullptr), capacity(0) {}
    arena_buffer(void* data, size_t size);
    ~arena_buffer();

    arena_buffer(arena_buffer&& other);
    arena_buffer(const arena_buffer& other);
    arena_buffer& operator=(arena_buffer&& other);
    arena_buffer& operator=(const arena_buffer& other);

    void allocate(uint64_t size);
    void extend();
    void release();
    void zero_mem();
    void zero_range(uint64_t offset, uint64_t size);

    void write(const void* data, size_t size);

    template <typename T>
      requires std::is_trivially_copyable_v<T>
    void write(const T& value) {
      if (offset + sizeof(T) > capacity) {
        extend();
      }
      element_sizes.push_back(sizeof(T));

      T* obj = new (memory_cursor) T(value);
      memory_cursor = static_cast<uint8_t*>(memory_cursor) + sizeof(T);
      offset += sizeof(T);
    }

    template <typename T>
    size_t get_offset(size_t idx) const {
      OTHER_ASSERT(idx < element_sizes.size(), "Attempting to retrieve invalid index! expected {} > {} num elements", idx, element_sizes.size());
      OTHER_ASSERT(sizeof(T) == element_sizes[idx], "Attempting to access buffer with invalidly sized type {}! expected size {} != {} stored size", typeid(T).name(), sizeof(T), element_sizes[idx]);

      size_t offset = std::accumulate(element_sizes.begin(), element_sizes.begin() + idx, 0);
      return offset;
    }

    /// @note we want both the mutable and non-mutable versions of At and when
    ///         the buffer is passed by const reference, we want to ensure that
    ///         we don't infinitely recurse 'const T& At(idx) const' so need both of these functions

    template <typename T>
    T& at(size_t index) {
      size_t offset = get_offset<T>(index);
      return *unchecked_ptr_at<T>(offset);
    };

    template <typename T>
    const T& at(size_t index) const {
      size_t offset = get_offset<T>(index);
      return *unchecked_ptr_at<T>(offset);
    };

    template <typename T>
    T* ptr_at(size_t index) {
      size_t offset = get_offset<T>(index);
      return unchecked_ptr_at<T>(offset);
    }

    template <typename T>
    const T* ptr_at(size_t index) const {
      size_t offset = get_offset<T>(index);
      return unchecked_ptr_at<T>(offset);
    }

   private:
    void* memory_start = nullptr;
    void* memory_cursor = nullptr;

    uint64_t offset = 0;
    uint64_t capacity = 0;
    std::vector<uint64_t> element_sizes;

    template <typename T>
    T* unchecked_ptr_at(size_t offset) {
      return std::launder(reinterpret_cast<T*>(memory_at(offset)));
    }
    template <typename T>
    const T* unchecked_ptr_at(size_t offset) const {
      return std::launder(reinterpret_cast<const T*>(memory_at(offset)));
    }

    void* memory_at(uint64_t offset);
    const void* memory_at(uint64_t offset) const;
  };

}  // namespace other

#endif  // OTHER_CORE_ARENA_BUFFER_HPP