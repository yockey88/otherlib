# Subsystem Model (Draft)

## Declaring a Subsystem

```cpp
// core/logger.hpp
namespace other {
  class logger : public subsystem<logger> {
    /// ...
  };
}

OTHER_SUBSYSTEM(other::logger)
```

The `OTHER_SUBSYSTEM(T)` macro generates the following:

```cpp
template <>                                                                
struct other::subsystem_description<T> {                                   
  static constexpr size_t size = sizeof(T);                                
  static constexpr size_t alignment = alignof(T);                          
  static inline subsystem_storage_t<T> storage;                            
  static T* ptr() { return std::launder(reinterpret_cast<T*>(&storage)); } 
  static void* address() { return reinterpret_cast<void*>(&storage); }     
};
```

## Extending the Model

1. Forward-declare the type in an accessible header.
2. Apply `OTHER_SUBSYSTEM(MySubsystem)` in a translation unit.
3. Provide `initialize()` and `shutdown()` methods following existing conventions.
4. Update `other::shutdown_subsystems` to call `subsystem<MySubsystem>::shutdown()` in the correct order.
5. Add configuration hooks through `config_table` and ensure logging/profiling is in place.

## Subsystems (at time of writing)

| Subsystem | Header | Role |
| --- | --- | --- |
| `arena` | `other-core/src/core/arena.hpp` | Primary allocation arena for the runtime. |
| `logger` | `other-core/src/core/logger.hpp` | Central logging registry backed by spdlog sinks. |
| `renderer_backend` | `other-renderer/src/renderer/renderer_backend.hpp` | Loads and manages rendering APIs. |
| `scripting_environment` | `other-scripting/src/script/scripting_environment.hpp` | Hosts .NET runtime, python, and lua script bindings. |
| `type_database` | `other-scripting/src/dotnet/type_cache.hpp` | Reflection cache bridging native and managed types. |
