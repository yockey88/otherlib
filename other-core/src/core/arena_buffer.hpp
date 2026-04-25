/**
 * \file core/arena_buffer.hpp
 **/
#ifndef OTHER_CORE_ARENA_BUFFER_HPP
#define OTHER_CORE_ARENA_BUFFER_HPP

#include <numeric>

#include <glm/glm.hpp>

#include "core/defines.hpp"
#include "core/logger.hpp"
#include "serialization//reflection.hpp"

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

    inline void* data() { return memory_start; }
    inline size_t size() const { return offset; }
    inline size_t max_size() const { return capacity; }
    inline size_t num_elements() const { return element_sizes.size(); }

    inline size_t element_size(size_t idx) const {
      OTHER_ASSERT(idx < element_sizes.size(), "Attempting to access element size at index {} but only {} elements exist", idx, element_sizes.size());
      return element_sizes[idx];
    }

    std::string dump_buffer() const;

    void allocate(uint64_t size);
    void extend();
    void release();
    void zero_mem();
    void zero_range(uint64_t offset, uint64_t size);

    void write(const void* data, size_t size);

    template <typename T>
      requires is_container<T>
    void write_arr(const T& container) {
      allocate(sizeof(typename T::value_type) * container.size());
      for (const auto& item : container) {
        buffer_data(item);
      }
    }

    template <typename T>
    size_t buffer_data(const T& value) {
      size_t index = element_sizes.size();

      if (capacity == 0) {
        if constexpr (std::same_as<T, std::string> || std::same_as<T, std::string_view>) {
          /// arbitrary, but assume use of this function means we're gonna want more than one
          allocate((value.length() + 1) * 64);
        } else {
          allocate(sizeof(value) * 64);
        }
      }

      if constexpr (std::same_as<T, std::string> || std::same_as<T, std::string_view>) {
        if (offset + value.length() + 1 > capacity) {
          extend();
        }

        for (size_t i = 0; i < value.length(); ++i) {
          byte_at(offset + i) = static_cast<uint8_t>(value[i]);
        }
        byte_at(size() + value.length()) = '\0';

        shift_cursor(value.length() + 1);
        element_sizes.push_back(value.length() + 1);
      } else {
        if (offset + sizeof(T) > capacity) {
          extend();
        }

        size_t sz = sizeof(T);

        *reinterpret_cast<T*>(memory_cursor) = value;
        shift_cursor(sz);
        element_sizes.push_back(sz);
      }

      return index;
    }

    template <typename T>
    size_t get_offset(size_t idx) const {
      OTHER_ASSERT(idx < element_sizes.size(), "Attempting to retrieve invalid index! expected {} > {} num elements", idx, element_sizes.size());
      OTHER_ASSERT(sizeof(T) <= element_sizes[idx], "Attempting to access buffer with invalidly sized type {}! requested size {} > {} stored size", typeid(T).name(), sizeof(T), element_sizes[idx]);
      if (sizeof(T) < element_sizes[idx]) {
        CORE_LOG_WARN("Accessing buffer with type {} that is smaller than stored size {}. This may lead to undefined behavior.", typeid(T).name(), element_sizes[idx]);
      }

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

    const std::span<uint8_t> view_bytes() const {
      return std::span<uint8_t>(static_cast<uint8_t*>(memory_start), offset);
    }

   private:
    void* memory_start = nullptr;
    void* memory_cursor = nullptr;

    uint64_t offset = 0;
    uint64_t capacity = 0;
    std::vector<uint64_t> element_sizes;

    void shift_cursor(uint64_t size);

    template <typename T>
    T* unchecked_ptr_at(size_t offset) {
      return std::launder(reinterpret_cast<T*>(memory_at(offset)));
    }
    template <typename T>
    const T* unchecked_ptr_at(size_t offset) const {
      return std::launder(reinterpret_cast<const T*>(memory_at(offset)));
    }

    uint8_t& byte_at(uint64_t offset);
    const uint8_t& byte_at(uint64_t offset) const;

    void* memory_at(uint64_t offset);
    const void* memory_at(uint64_t offset) const;
  };

}  // namespace other

#endif  // OTHER_CORE_ARENA_BUFFER_HPP