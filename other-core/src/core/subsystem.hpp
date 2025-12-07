/**
 * \file core/subsystem.hpp
 **/
#ifndef OTHER_CORE_SUBSYSTEM_HPP
#define OTHER_CORE_SUBSYSTEM_HPP

#include <concepts>
#include <format>
#include <memory>
#include <mutex>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "core/profiler.hpp"

namespace other {

  template <typename T>
  struct subsystem_description;
  template <typename T>
  using subsystem_storage_t = std::aligned_storage_t<subsystem_description<T>::size, subsystem_description<T>::alignment>;

  template <typename T>
  struct subsystem_deleter {
    void operator()(T* ptr) {
      if (ptr != nullptr) {
        // If T is trivially destructible, we do not need to call the destructor and we can just zero the memory for safety
        if constexpr (!std::is_trivially_destructible_v<T>) {
          std::destroy_at(ptr);
        }
        std::memset(ptr, 0, subsystem_description<T>::size);
      }
    }
  };

  template <typename T>
  concept is_subsystem =
    requires(T t) {
      { subsystem_description<T>::size } -> std::convertible_to<size_t>;
      { subsystem_description<T>::alignment } -> std::convertible_to<size_t>;
      { new (&subsystem_description<T>::storage) T() } -> std::same_as<T*>;
      { subsystem_description<T>::ptr() } -> std::convertible_to<T*>;
      { subsystem_description<T>::address() } -> std::convertible_to<void*>;
      { subsystem_deleter<T>()(std::declval<T*>()) } -> std::same_as<void>;
    };

  template <typename T>
  class subsystem {
   public:
    static std::mutex subsystem_mtx;

    static void set(T* obj) {
      PROFILE_SECTION("subsystem<>::set");
      if (obj == nullptr) {
        throw std::runtime_error("Cannot set subsystem instance to null.");
      }
      std::lock_guard lock(subsystem_mtx);
      instance = obj;

      if constexpr (requires(T t) { { T::on_set(std::declval<T*>()) } -> std::same_as<void>; }) {
        T::on_set(obj);
      }
    }

    static void initialize() {
      PROFILE_SECTION("subsystem<>::initialize");
      if (instance == nullptr) {
        std::lock_guard lock(subsystem_mtx);
        new (&subsystem_description<T>::storage) T();
        instance = std::launder(reinterpret_cast<T*>(&subsystem_description<T>::storage));
      }
    }

    static void shutdown() {
      PROFILE_SECTION("subsystem<>::shutdown");
      std::lock_guard lock(subsystem_mtx);
      subsystem_deleter<T>()(instance);
      instance = nullptr;
    }

    static T* get() {
      PROFILE_SECTION("subsystem<>::get");
      if (instance == nullptr) {
        initialize();
      }
      return instance;
    }

    std::string as_string() const {
      constexpr bool has_description_as_string =
        requires(const T& t) { { subsystem_description<T>::as_string(t) } -> std::convertible_to<std::string>; };
      constexpr bool has_instance_as_string =
        requires(const T& t) { { t.as_string() } -> std::convertible_to<std::string>; };

      constexpr static bool has_to_string = has_description_as_string || has_instance_as_string;

      if constexpr (has_to_string) {
        if constexpr (has_description_as_string) {
          return subsystem_description<T>::as_string(*this);
        } else if constexpr (has_instance_as_string) {
          return instance->as_string();
        } else {
          return std::format("Subsystem<{}> [no as-string method available]", typeid(T).name());
        }
      } else {
        return std::format("Subsystem<{}> [no as-string method available]", typeid(T).name());
      }
    }

   protected:
    subsystem() = default;

   private:
    static T* instance;

    subsystem(subsystem&&) = delete;
    subsystem(const subsystem&) = delete;
    subsystem& operator=(subsystem&&) = delete;
    subsystem& operator=(const subsystem&) = delete;
  };
  template <typename T>
  T* subsystem<T>::instance = nullptr;
  template <typename T>
  std::mutex subsystem<T>::subsystem_mtx;

}  // namespace other

#define OTHER_SUBSYSTEM(T)                                                   \
  template <>                                                                \
  struct other::subsystem_description<T> {                                   \
    static constexpr size_t size = sizeof(T);                                \
    static constexpr size_t alignment = alignof(T);                          \
    static inline subsystem_storage_t<T> storage;                            \
    static T* ptr() { return std::launder(reinterpret_cast<T*>(&storage)); } \
    static void* address() { return reinterpret_cast<void*>(&storage); }     \
  };

#endif  // OTHER_CORE_SUBSYSTEM_HPP