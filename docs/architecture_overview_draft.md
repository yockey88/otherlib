# Other Environment Architectural Overview

> Status: Draft for review. Links point to supporting notes that expand on the topics outlined here.

## Purpose and Scope

Other ENvironment is a C++23 game and simulation framework that supplies an opinionated runtime, tooling pipeline, and cross-language scripting bridge. When linked into an application, Other Environment owns process startup, configures subsystems, and executes project-specific drivers. This document sketches the architectural shape of the repository so that new contributors can navigate the codebase, understand layering, and spot the integration points for new features.

Supporting detail:

- [Runtime Flow](architecture/runtime_flow_draft.md)
- [Subsystem Model](architecture/subsystem_model_draft.md)

## High-Level Layering

```text
+-------------------------------------------------------------+
|                   Application / Drivers                     |
|  (development-drivers, plugins, tools, tests, examples)     |
+----------------------------v--------------------------------+
|                Other Environment Integration                |
|  (otherlib/src, entry point, project + plugin systems)      |
+----------------------------v--------------------------------+
|           Domain Modules (C++ and .NET components)          |
|  Rendering | Scene | Scripting | Networking | Tooling       |
+----------------------------v--------------------------------+
|                  Foundation and Runtime Core                |
|  Core utilities, memory, config, logging, threading         |
+----------------------------v--------------------------------+
|                External Dependencies (extern/)              |
+-------------------------------------------------------------+
```

### Layer Guidelines

- **Foundation** (`other-core`, parts of `extern/`): cross-cutting utilities with minimal dependencies. Higher layers depend on these but never the other way around.
- **Domain Modules** (`other-renderer`, `other-scene`, `other-network`, `other-scripting`, `other-csharp`, `other-csharp-interop`, `tools/`): pair the core with rendering, asset, and scripting systems. Modules collaborate through well-defined interfaces and global subsystems.
- **Integration** (`otherlib/`): composes modules into the shipped runtime, manages subsystem lifetimes, and exposes entry points to drivers.
- **Drivers and Applications** (`development-drivers/`, `driver/`, user binaries): consume OtherLib APIs, usually via the provided `other_main` hook.

## Repository Surface Map

| Area | Role | Notes |
| --- | --- | --- |
| `cmake/` | Build recipes and install helpers | `add_other_driver.cmake` exports convenience macros; `version.hpp.in` feeds package metadata. |
| `extern/` | Third-party libraries | SDL3, GLAD, Assimp, ImGui, EnTT, magic_enum, FlatBuffers, DotNet hosting, etc. |
| `other-core/` | Foundation runtime | Memory arenas, logging, config, math, serialization, command processing, subsystem infrastructure. |
| `other-renderer/` | Rendering stack | Renderer backend abstraction, GPU resource lifecycle, render graph + pipelines, UI helpers. |
| `other-scene/` | Scene and asset systems | Scene graph, asset pipeline, serialization, component descriptors. |
| `other-network/` | Networking | Session state machine, networking thread, packet handling hooks. |
| `other-scripting/` | Language bindings | Dotnet and Python bridges, script lifecycle management, native bindings. |
| `other-csharp-interop/` | Managed/Native bridge | C# host helpers, assembly loader, managed object wrappers. |
| `other-csharp/` | Managed runtime surface | C# utilities, renderer and scene bindings, generator tooling. |
| `otherlib/` | Runtime integrator | Provides `main`, wires subsystems, plugin support, project serialization. |
| `development-drivers/` | Example executables | Runtime, renderer, server, simulation drivers for development and testing. |
| `tools/` | Standalone utilities | Build tools, mesh baker, project writer; exercise APIs outside the main runtime. |

## Core Concepts

### Subsystem Pattern

The subsystem framework in `other-core/src/core/subsystem.hpp` owns singleton-like services (arena, logger, renderer backend, scripting environment). Subsystems expose:

- Static storage with compile-time size/alignment metadata
- Explicit `initialize` and `shutdown` hooks
- Optional `on_set` callbacks for custom wiring

See [Subsystem Model](architecture/subsystem_model_draft.md) for lifecycle, access rules, and extension guidance.

### Memory Arenas and Allocators

`other-core` emphasises owned arenas (`arena.hpp`, `arena_allocator.hpp`) to avoid heap churn. Practically, most high-frequency allocations route through the primary arena established at startup (`otherlib/src/other.cpp::initialize_primary_arena`). Specialized pools provide deterministic lifetimes for small objects (`memory_pool.hpp`, `arena_buffer.hpp`). Modules consuming these allocators should respect the arena boundaries to avoid leaks during hot reload or shutdown.

### Command and Configuration Flow

Runtime bootstrapping funnels through `core/command_line` and `core/config_table`. Command-line parsing populates diagnostics, working directory, and config file location. Config tables integrate TOML/JSON data and feed defaults into subsystems (logging, scripting, rendering backend selection). Drivers receive the fully parsed command line plus resolved config.

### Logging, Profiling, Diagnostics

`core/logger` wraps spdlog sinks. `register_log_sinks` selects console and file sinks based on the config, while profiling macros in `core/profiler.hpp` (Tracy integration) instrument critical sections, including the engine entry point.

## Module Walkthrough

### other-core

Responsibility: foundational utilities and runtime primitives.

Notable namespaces:

- `core/` – subsystem definitions, command processing, configuration, logging, arena management, reference counting, timekeeping.
- `data-structures/` – arena-backed containers, graph utilities.
- `math/` – vector/matrix operations, random utilities, morton codes, integrators.
- `event/` – event bus for cross-module signalling.
- `serialization/` – reflection-powered serializers, parser combinators.
- `thread/` – message bus, worker threads, channel abstraction.

Dependencies: minimal; only relies on CRT, STL, and headers from `extern/` as needed.

### other-renderer

Responsibility: rendering backend abstraction and GPU resource management.

Key components:

- `renderer_backend` subsystem handles API selection and lifecycle.
- `renderer/` contains draw command composition, render graph scheduling, window manager, backend implementations (OpenGL currently).
- `gpu_resource/` defines buffers, textures, framebuffers, with loaders tying into `other-scene` assets.
- `model/` and `pipelines/` support asset import and pipeline configuration for renderer features.
- `ui/` provides higher-level widgets layered over ImGui integration.

The renderer depends heavily on `other-core` math, logging, and arena subsystems.

### other-scene

Responsibility: asset ingestion, scene graph, component serialization.

Highlights:

- `asset/` pipeline orchestrates source-to-runtime conversion, leverages FlatBuffers and custom serialization.
- `scene/` modules manage scene tree, storage, spatial structures (octree), and runtime data.
- `object/` extends scene graph with materials, scripts, and render components, forming the cross-module link to rendering and scripting.

### other-network

Responsibility: asynchronous networking services.

- `network_thread` orchestrates IO threads, using Asio from `extern/`.
- `session` and `session_state_machine` manage connection state and message dispatch.
- `network-packets/` (currently placeholder) intended for protocol message definitions.

The network module integrates with the core event system to surface network events to higher layers.

### other-scripting and Interop Modules

`other-scripting` hosts language bindings and script execution infrastructure:

- `dotnet/` manages CLR hosting, assemblies, native/managed marshaling, and garbage collector hooks.
- `python/` and `lua/` directories provide placeholders for additional scripting runtimes.
- `script/` layer offers runtime script objects, environment management, and bridging to engine subsystems.

`other-csharp-interop` supplies managed interop helpers: attribute metadata, native arrays, function registration, and host wrappers. This project compiles into a .NET assembly referenced by the C++ runtime.

`other-csharp` contains the managed library used by game projects. It exposes renderer helpers, scene bindings, and tools implemented in C#.

Together, these projects enable scripts to call back into native services defined in `other-core`, `other-renderer`, and `other-scene`.

### otherlib

Responsibility: orchestrate the engine runtime and surface the public API.

Key files:

- `other.cpp` – real entry point. Parses command line, loads configuration, registers log sinks, initializes subsystems (arena, renderer backend, scripting environment), invokes `other_main`, and handles shutdown.
- `other.hpp` – exported API for clients, re-exporting common headers and exposing helper functions (`initialize_other_environment`, `shutdown_other_environment`).
- `driver/`, `plugin/`, `project/` – frameworks for drivers, plugin loading, and project serialization.

`otherlib` is the layer most application developers interact with. It ensures a consistent startup pipeline and packaging story via CMake install rules.

### Development Drivers and Tools

`development-drivers/` hosts runnable examples that exercise focused areas (rendering dev driver, runtime driver, server driver, simulation driver). These drivers are invaluable references for wiring new subsystems or verifying integration changes.

`tools/` contains standalone utilities (mesh baker, project writer, build tool) that operate on engine data in offline workflows. They lean on `other-core` and domain modules without initializing the full runtime.

## Runtime Execution Overview

A simplified call graph for the typical executable produced by `add_static_driver`:

```text
WinMain / main (generated)      [otherlib]
  -> other::entry               [otherlib/src/other.cpp]
       -> initialize_primary_arena()
       -> command_line::parse()
       -> config_table::load()
       -> register_log_sinks()
       -> renderer_backend::load_backend()
       -> bind_primary_scripting_environment()
       -> bind_environment_scripts()
       -> other_main(cmd, config)   <-- user driver
       -> renderer_backend::unload_backend()
       -> cleanup_scripting_environment()
       -> shutdown_subsystems()
```

More detail lives in [Runtime Flow](architecture/runtime_flow_draft.md), including optional execution paths such as headless or custom `main` scenarios.

## Data and Asset Pipeline

- **Asset Import**: `other-scene/asset` uses configurable pipelines and FlatBuffers schemas (`development-drivers/simulation/resources/sim-config-spec.fbs` is an example). Imported data ends up in runtime-friendly structures stored in arena-backed containers.
- **Rendering Integration**: Assets produce GPU resource descriptors consumed by `other-renderer/gpu_resource`. The render graph composes passes referencing these resources.
- **Scripting Hooks**: Script components (`other-scene/object/script_component`) attach managed behaviours. The scripting environment ensures .NET assemblies are loaded and binds native functions exposed via `other-scripting`.

## Concurrency and Messaging

- **Threading**: `other-core/thread` defines worker threads and message buses. The networking module dedicates threads to IO, while other systems may schedule work items through the core worker infrastructure.
- **Events**: `other-core/event` exposes an event system for decoupled notifications. Subsystems emit events (e.g., window events, network packets) that drivers can subscribe to.

## External Integration

OtherLib statically or dynamically links against a curated external stack:

- **Rendering**: SDL3 for windowing/input, GLAD for OpenGL loading, Assimp for model import, ImGui for debug UI.
- **Serialization/Data**: FlatBuffers, JSON, TOML++ for configuration.
- **Utility**: EnTT (ECS patterns), magic_enum, Tracy profiler, nativefiledialog for desktop integrations.
- **Scripting**: .NET hosting, PyBind11 scaffolding, Python runtime.

CMake setup under `extern/` provides per-configuration libraries (Debug/Release). Install rules mirror this layout for redistributable builds.

## Extension Points

- **Drivers**: Implement `other_main` (default) or provide a custom `main` with `OTHER_DISABLE_MAIN`. Drivers can opt for static or dynamic entry via `add_static_driver` / `add_dynamic_driver` macros.
- **Subsystems**: New subsystems should conform to the pattern in `subsystem.hpp` and register with `shutdown_subsystems` in `other.cpp`.
- **Plugins**: The `otherlib/src/plugin` area outlines loadable plugin architecture. Plugins should expose standardized entry points recognized by the runtime loader.
- **Scripting**: Add new bindings under `other-scripting/bindings` and surface them through the .NET interop layer to make them accessible to managed code.

## Build and Packaging Model

- **CMake**: Root `CMakeLists.txt` configures build types, platform defines, selects external libs, and adds subdirectories per module. Custom options toggle development drivers, tests, and example applications.
- **Installation**: `cmake/other_install.cmake` and related helpers package headers, libs, and assemblies into `C:/OtherEnvironment` by default. External headers/libraries are re-installed alongside internal artifacts.
- **Configuration Macros**: Build type and platform macros (`OTHER_ENVIRONMENT_DEBUG`, `OTHER_ENVIRONMENT_WINDOWS`, etc.) propagate via `target_compile_definitions` so that runtime code can select behaviour at compile time.

## Documentation Outlook

Immediate priorities for deeper technical documentation:

1. Finalize this draft with module owners to ensure coverage accuracy.
2. Expand [Runtime Flow](architecture/runtime_flow_draft.md) with sequence diagrams for rendering and scripting startup.
3. Flesh out [Subsystem Model](architecture/subsystem_model_draft.md) with examples of registering new subsystems and integrating with shutdown logic.
4. Add targeted how-to guides (e.g., "Adding a Rendering Backend", "Authoring a Scripting Binding") once foundational docs solidify.

Feedback welcome—annotate this draft or open issues with clarifications and corrections.
