/**
 * @file data-structures/arena_vector.hpp
 */
#ifndef OTHER_CORE_DATA_STRUCTURES_ARENA_VECTOR_HPP
#define OTHER_CORE_DATA_STRUCTURES_ARENA_VECTOR_HPP

#include "core/arena_allocator.hpp"
#include "core/defines.hpp"

namespace other {

  template <typename T>
  class arena_vector {
   public:
    arena_vector() = default;
    arena_vector(arena_vector&&) = default;
    arena_vector(const arena_vector&) = delete;
    arena_vector& operator=(arena_vector&&) = default;
    arena_vector& operator=(const arena_vector&) = delete;

    arena_vector(T* data, natural_t size, natural_t capacity)
        : data(data), size(size), capacity(capacity) {}
    arena_vector(natural_t capacity)
        : data(nullptr), size(0), capacity(capacity) {}
    arena_vector(T* data, natural_t size)
        : data(data), size(size), capacity(size) {}

    ~arena_vector() = default;

    T& operator[](natural_t i) { return data[i]; }
    const T& operator[](natural_t i) const { return data[i]; }

    void reserve(natural_t new_size) {
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
      if (new_size * sizeof(T) > capacity) {
        T* new_data = allocator.allocate_block(new_size);

        for (natural_t i = 0; i < size; i++) {
          new_data[i] = std::move(data[i]);
        }

        allocator.free_block(data, size);
        data = new_data;
        capacity = new_size * sizeof(T);
      }
    }

    struct iterator {
      T* ptr;
      iterator& operator++() {
        ptr++;
        return *this;
      }
      bool operator!=(const iterator& other) const {
        return ptr != other.ptr;
      }
      T& operator*() {
        return *ptr;
      }

      constexpr auto operator<=>(const iterator& other) const = default;
    };

    iterator begin() { return iterator{ data }; }
    iterator end() { return iterator{ data + size }; }

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

}  // namespace other

#endif  // OTHER_CORE_DATA_STRUCTURES_ARENA_VECTOR_HPP