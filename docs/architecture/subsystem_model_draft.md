# Subsystem Model (Draft)

This note clarifies how global subsystems are declared, initialized, and shut down across OtherLib. It complements the [Architectural Overview](../architecture_overview_draft.md) and [Runtime Flow](runtime_flow_draft.md) drafts.

## Terminology

- **Subsystem**: A singleton-like service exposed through `other::subsystem<T>` (e.g., `arena`, `logger`, `renderer_backend`).
- **Description**: Traits struct (`subsystem_description<T>`) containing size, alignment, and optional utility helpers for the subsystem instance.
- **Storage**: Backing memory reserved via `std::aligned_storage_t` defined by the `OTHER_SUBSYSTEM(T)` macro.

## Declaring a Subsystem

```cpp
// core/logger.hpp
namespace other {
  class logger;
}

OTHER_SUBSYSTEM(other::logger)
```

Key points:

- The macro defines static storage with correct alignment and a `ptr()` accessor.
- `subsystem<T>::instance` keeps a single pointer to the active instance.
- Construction happens lazily the first time `subsystem<T>::get()` is called (unless manual initialization occurs earlier).

## Lifecycle Operations

### Initialization

```cpp
other::subsystem<other::logger>::initialize();
```

- Placement-new constructs the object inside the reserved storage.
- If `T` declares a static `on_set` function, it is invoked immediately after initialization, enabling registration of global callbacks.

### Access

```cpp
auto* log = other::subsystem<other::logger>::get();
log->create_logger("other-core-log", spdlog::level::trace);
```

- `get()` lazily initializes when necessary and guarantees a non-null pointer upon return.
- Accessors should be cheap; repeated lookups are acceptable inside hot loops.

### Shutdown

```cpp
other::subsystem<other::logger>::shutdown();
```

- `subsystem_deleter` either invokes the destructor or zeroes the storage for trivially destructible types.
- Shutdown order matters. `otherlib/src/other.cpp::shutdown_subsystems` encodes the canonical order; extend it when introducing new subsystems.

## Composition Guidelines

- Keep constructors lightweight. Heavy work should happen in explicit `initialize` methods invoked by the runtime.
- Avoid storing owning pointers to other subsystems during construction. Prefer method-level access to prevent static initialization order issues.
- Document thread-safety expectations. Some subsystems (e.g., `renderer_backend`) assume single-thread access, whereas others (e.g., `message_bus`) may support concurrency.
- Expose `as_string` helpers when diagnostic output enhances debugging. The subsystem template will use either `subsystem_description<T>::as_string` or `T::as_string` if present.

## Extending the Model

1. Forward-declare the type in an accessible header.
2. Apply `OTHER_SUBSYSTEM(MySubsystem)` in a translation unit.
3. Provide `initialize()` and `shutdown()` methods following existing conventions.
4. Update `other::shutdown_subsystems` to call `subsystem<MySubsystem>::shutdown()` in the correct order.
5. Add configuration hooks through `config_table` and ensure logging/profiling is in place.

## Known Subsystems

| Subsystem | Header | Role |
| --- | --- | --- |
| `arena` | `other-core/src/core/arena.hpp` | Primary allocation arena for the runtime. |
| `logger` | `other-core/src/core/logger.hpp` | Central logging registry backed by spdlog sinks. |
| `renderer_backend` | `other-renderer/src/renderer/renderer_backend.hpp` | Loads and manages rendering APIs. |
| `scripting_environment` | `other-scripting/src/script/scripting_environment.hpp` | Hosts .NET runtime and script bindings. |
| `type_database` | `other-scripting/src/dotnet/type_cache.hpp` | Reflection cache bridging native and managed types. |

(The list is non-exhaustive; consult module headers for additional subsystems.)

## Future Documentation

- Formalize dependency graph between subsystems to validate shutdown ordering.
- Provide examples for thread-aware subsystems (e.g., `message_bus`, `worker_thread`).
- Summarize testing strategies for subsystem-level features.
