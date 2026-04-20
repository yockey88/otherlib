/**
 * @file data-structures/arena_vector.hpp
 */
#ifndef OTHER_CORE_DATA_STRUCTURES_ARENA_VECTOR_HPP
#define OTHER_CORE_DATA_STRUCTURES_ARENA_VECTOR_HPP

#include <iterator>
#include <ranges>

#include "core/arena_allocator.hpp"
#include "core/defines.hpp"
#include "core/logger.hpp"

namespace other {

  template <typename T>
  class arena_vector {
   public:
    arena_vector() {
      data = nullptr;
      size = 0;
      capacity = 0;
      initial_allocation();
    }
    arena_vector(arena_vector&&) = default;
    arena_vector(const arena_vector&) = delete;
    arena_vector& operator=(arena_vector&&) = default;
    arena_vector& operator=(const arena_vector&) = delete;

    arena_vector(T* data, natural_t size, natural_t capacity)
        : data(data), size(size), capacity(capacity) {
      OTHER_ASSERT(size <= capacity, "Size of arena_vector cannot exceed its capacity.");
      OTHER_ASSERT(capacity == 0 || data != nullptr, "Data pointer cannot be null if capacity is greater than zero.");
    }
    arena_vector(natural_t capacity)
        : data(nullptr), size(0), capacity(capacity) {
      if (capacity > 0) {
        realloc_with_capacity(capacity);
      }
    }
    arena_vector(T* data, natural_t size)
        : data(data), size(size), capacity(size) {}

    ~arena_vector() = default;

    T& operator[](natural_t i) { return data[i]; }
    const T& operator[](natural_t i) const { return data[i]; }

    inline bool empty() const { return size == 0; }
    inline natural_t get_size() const { return size; }
    inline natural_t get_capacity() const { return capacity; }

    void reserve(natural_t new_size) {
      if (new_size == 0) {
        clear();
        return;
      }

      realloc_with_capacity(new_size);
      size = 0;
    }

    void clear() {
      allocator.free_block(data, size);
      size = 0;
      data = nullptr;
      capacity = 0;
    }

    void push_back(T&& value) {
      resize_if_necessary();
      data[size++] = std::move(value);
    }

    void push_back(const T& value) {
      resize_if_necessary();
      data[size++] = value;
    }

    void pop_back() {
      if (size > 0) {
        size--;
      }
    }

    T& emplace_back() {
      resize_if_necessary();
      data[size++] = T();
      return data[size - 1];
    }

    T& emplace_back(T&& value) {
      resize_if_necessary();
      data[size++] = std::move(value);
      return data[size - 1];
    }

    template <typename... Args>
      requires std::is_constructible_v<T, Args...>
    T& emplace_back(Args&&... args) {
      resize_if_necessary();
      data[size++] = T(std::forward<Args>(args)...);
      return data[size - 1];
    }

    void realloc_with_capacity(natural_t new_size) {
      T* new_data = allocator.allocate_block(new_size);

      for (natural_t i = 0; i < size; i++) {
        new_data[i] = std::move(data[i]);
      }

      allocator.free_block(data, size);
      data = new_data;
      capacity = new_size * sizeof(T);
    }

    class iterator {
     public:
      using iterator_category = std::random_access_iterator_tag;
      using difference_type = std::ptrdiff_t;
      using value_type = T;
      using reference = T&;
      using const_reference = reference;
      using pointer = T*;
      using const_pointer = pointer;

      iterator() = default;
      iterator(const iterator&) = default;
      iterator& operator=(const iterator&) = default;

      iterator(T* ptr)
          : ptr(ptr) {}
      ~iterator() = default;

      reference operator*() noexcept { return *ptr; }
      const_reference operator*() const noexcept { return *ptr; }

      reference operator[](difference_type offset) noexcept { return *(ptr + offset); }
      const_reference operator[](difference_type offset) const noexcept { return *(ptr + offset); }

      pointer operator->() noexcept { return ptr; }
      const_pointer operator->() const noexcept { return ptr; }

      friend iterator operator+(difference_type offset, const iterator& it) { return iterator{ it.ptr + offset }; }
      iterator operator+(difference_type offset) const { return iterator{ ptr + offset }; }
      iterator operator+=(difference_type offset) {
        ptr += offset;
        return *this;
      }
      iterator& operator++() {
        ptr++;
        return *this;
      }
      iterator operator++(int) {
        iterator temp = *this;
        ptr++;
        return temp;
      }

      friend iterator operator-(difference_type offset, const iterator& it) { return iterator{ it.ptr - offset }; }
      difference_type operator-(difference_type offset) const { return iterator{ ptr - offset }; }
      iterator operator-=(difference_type offset) {
        ptr -= offset;
        return *this;
      }
      iterator& operator--() {
        ptr--;
        return *this;
      }
      iterator operator--(int) {
        iterator temp = *this;
        ptr--;
        return temp;
      }

      bool operator==(const iterator& other) const { return ptr == other.ptr; }
      bool operator!=(const iterator& other) const { return ptr != other.ptr; }
      bool operator<(const iterator& other) const { return ptr < other.ptr; }
      bool operator<=(const iterator& other) const { return ptr <= other.ptr; }
      bool operator>(const iterator& other) const { return ptr > other.ptr; }
      bool operator>=(const iterator& other) const { return ptr >= other.ptr; }

     private:
      friend class arena_vector<T>;
      T* ptr;
    };

    /// C++ range functions support
    iterator begin() { return iterator{ data }; }
    iterator begin() const { return iterator{ data }; }
    iterator end() { return iterator{ data + size }; }
    iterator end() const { return iterator{ data + size }; }

    iterator find(const T& predicate) {
      for (natural_t i = 0; i < size; i++) {
        if constexpr (requires { predicate == data[i]; }) {
          if (predicate == data[i]) {
            return iterator{ data + i };
          }
        } else {
          static_assert(false, "Predicate must be comparable or callable with the element type.");
        }
      }
      return end();
    }

    template <typename Fn>
    iterator find_if(Fn&& predicate) {
      for (natural_t i = 0; i < size; i++) {
        if (predicate(data[i])) {
          return iterator{ data + i };
        }
      }
      return end();
    }

    iterator erase(const iterator& it) {
      if (it < begin() || it >= end()) {
        return end();
      }
      natural_t index = it.ptr - data;
      for (natural_t i = index; i < size - 1; i++) {
        data[i] = std::move(data[i + 1]);
      }
      size--;
      return iterator{ data + index };
    }

    natural_t size = 0;
    natural_t capacity = 0;

   private:
    T* data = nullptr;

    static inline arena_allocator<T> allocator;

    void initial_allocation() {
      capacity = 10 * sizeof(T);
      data = (T*)allocator.allocate_block(capacity);
      for (auto i = 0; i < size; i++) {
        new (&data[i]) T();
      }
    }

    void resize_if_necessary(natural_t additional = 1) {
      if (size + additional > capacity) {
        if (capacity == 0) {
          initial_allocation();
        } else {
          realloc_with_capacity(capacity * 2);
        }
      }
    }
  };
  static_assert(std::ranges::range<arena_vector<int>>, "arena_vector should satisfy range requirements.");
  // static_assert(std::ranges::input_range<arena_vector<int>>, "arena_vector should satisfy input range requirements.");
  // static_assert(std::ranges::forward_range<arena_vector<int>>, "arena_vector should satisfy forward range requirements.");
  // static_assert(std::ranges::bidirectional_range<arena_vector<int>>, "arena_vector should satisfy random access range requirements.");

}  // namespace other

#endif  // OTHER_CORE_DATA_STRUCTURES_ARENA_VECTOR_HPP