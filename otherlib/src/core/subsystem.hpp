/**
 * \file core/subsystem.hpp
 **/
#ifndef OTHER_CORE_SUBSYSTEM_HPP
#define OTHER_CORE_SUBSYSTEM_HPP

#include <concepts>
#include <format>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

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
    static void set(T* obj) {
      if (obj == nullptr) {
        throw std::runtime_error("Cannot set subsystem instance to null.");
      }
      instance = obj;
    }

    static void initialize() {
      if (instance == nullptr) {
        new (&subsystem_description<T>::storage) T();
        instance = std::launder(reinterpret_cast<T*>(&subsystem_description<T>::storage));
      }
    }

    static void shutdown() {
      subsystem_deleter<T>()(instance);
      instance = nullptr;
    }

    static T* get() {
      if (instance == nullptr) {
        initialize();
      }
      return instance;
    }

    std::string as_string() const {
      constexpr bool has_description_as_string =
        requires(const T& t) {
          { subsystem_description<T>::as_string(t) } -> std::convertible_to<std::string>;
        };
      constexpr bool has_instance_as_string =
        requires(const T& t) {
          { t.as_string() } -> std::convertible_to<std::string>;
        };

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
      // if constexpr (requires(T t) {})
    }

   protected:
    static T* instance;

   private:
    subsystem(subsystem&&) = delete;
    subsystem(const subsystem&) = delete;
    subsystem& operator=(subsystem&&) = delete;
    subsystem& operator=(const subsystem&) = delete;
  };
  template <typename T>
  T* subsystem<T>::instance = nullptr;

}  // namespace other

#endif  // OTHER_CORE_SUBSYSTEM_HPP