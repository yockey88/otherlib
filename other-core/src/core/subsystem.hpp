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

  /**
   * List of subsystems
   *  - arena
   *  - logger
   *  - file_system
   *  - input_system
   *  - type_database
   *  - physics_environment
   *  - renderer_backend
   *  - scripting_environment
   *
   * Profiles:
   *  - core only: arena, logger, file_system, input_system, type_database
   *  - rendering environment: core + renderer_backend
   *  - physics environment: core + physics_environment
   *  - scripting environment: core + scripting_environment
   *  - headless environment: core + physics_environment, scripting_environment
   *  - full: core + physics_environment, renderer_backend, scripting_environment
   **/

  struct subsystem_profile {
    constexpr static std::string_view kLogger = "logger";
    constexpr static std::string_view kArena = "arena";
    constexpr static std::string_view kFileSystem = "file_system";
    constexpr static std::string_view kInputSystem = "input_system";
    constexpr static std::string_view kTypeDatabase = "type_database";
    constexpr static std::string_view kPhysicsEnvironment = "physics_environment";
    constexpr static std::string_view kRendererBackend = "renderer_backend";
    constexpr static std::string_view kScriptingEnvironment = "scripting_environment";

    constexpr static std::string_view kMinimalProfileName = "minimal";
    constexpr static std::string_view kMinimalRenderingProfileName = "minimal-rendering";
    constexpr static std::string_view kMinimalPhysicsProfileName = "minimal-physics";
    constexpr static std::string_view kMinimalScriptingProfileName = "minimal-scripting";
    constexpr static std::string_view kHeadlessProfileName = "headless";
    constexpr static std::string_view kFullProfileName = "full";

    constexpr static std::string_view kMinimalProfile[] = { kLogger, kArena, kFileSystem, kInputSystem, kTypeDatabase };
    constexpr static std::string_view kMinimalRenderingProfile[] = { kLogger, kArena, kFileSystem, kInputSystem, kTypeDatabase, kRendererBackend };
    constexpr static std::string_view kMinimalPhysicsProfile[] = { kLogger, kArena, kFileSystem, kInputSystem, kTypeDatabase, kPhysicsEnvironment };
    constexpr static std::string_view kMinimalScriptingProfile[] = { kLogger, kArena, kFileSystem, kInputSystem, kTypeDatabase, kScriptingEnvironment };
    constexpr static std::string_view kHeadlessProfile[] = { kLogger, kArena, kFileSystem, kInputSystem, kTypeDatabase, kPhysicsEnvironment, kScriptingEnvironment };
    constexpr static std::string_view kFullProfile[] = { kLogger, kArena, kFileSystem, kInputSystem, kTypeDatabase, kPhysicsEnvironment, kRendererBackend, kScriptingEnvironment };
  };

  template <typename T>
  class subsystem {
   public:
    static std::mutex subsystem_mtx;
    static bool inert;

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
        if (inert) {
          throw std::runtime_error(std::format("Subsystem {} is inert and cannot be initialized without an instance being set.", typeid(T).name()));
        }

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
      initialize();
      return instance;
    }

    std::string as_string() const {
      constexpr bool has_description_to_string =
        requires(const T& t) { { subsystem_description<T>::to_string(t) } -> std::convertible_to<std::string>; };
      constexpr bool has_instance_to_string =
        requires(const T& t) { { t.to_string() } -> std::convertible_to<std::string>; };
      constexpr bool has_instance_static_to_string =
        requires(const T& t) { { T::to_string(t) } -> std::convertible_to<std::string>; };

      constexpr static bool has_to_string = has_description_to_string || has_instance_to_string || has_instance_static_to_string;

      if constexpr (has_to_string) {
        if constexpr (has_description_to_string) {
          return subsystem_description<T>::to_string(*this);
        } else if constexpr (has_instance_to_string) {
          return ((T*)instance)->to_string();
        } else {
          return T::to_string(*instance);
        }
      } else {
        return std::format("Subsystem<{}> [no to-string method available]", typeid(T).name());
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
  inline T* subsystem<T>::instance = nullptr;
  template <typename T>
  inline std::mutex subsystem<T>::subsystem_mtx;
  template <typename T>
  // subsystems explicitly activated by subsystem registration
  inline bool subsystem<T>::inert = true;

  class config_table;

  template <typename T>
  void initialize_subsystem(const config_table* config) {
    subsystem<T>::get();
  }

  using subsystem_initializer = void (*)(const config_table*);
  using subsystem_shutdown = void (*)();
  struct subsystem_definition {
    std::string name;
    std::span<const std::string_view> depends_on;

    subsystem_initializer initialize_fn = nullptr;
    subsystem_shutdown shutdown_fn = nullptr;
  };

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

#define OTHER_DEPENDENT_SUBSYSTEM(T, ...)                                    \
  template <>                                                                \
  struct other::subsystem_description<T> {                                   \
    static constexpr size_t size = sizeof(T);                                \
    static constexpr size_t alignment = alignof(T);                          \
    static inline subsystem_storage_t<T> storage;                            \
    static T* ptr() { return std::launder(reinterpret_cast<T*>(&storage)); } \
    static void* address() { return reinterpret_cast<void*>(&storage); }     \
    static constexpr std::string_view dependency_names[] = { __VA_ARGS__ };  \
  };

#endif  // OTHER_CORE_SUBSYSTEM_HPP