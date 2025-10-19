# Runtime Flow (Draft)

This note expands on the execution pipeline managed by `otherlib/src/other.cpp`. It should be read alongside the high-level [Architectural Overview](../architecture_overview_draft.md).

## Bootstrapping Sequence

```text
main() / WinMain()
  -> other::entry(argc, argv)
       1. initialize_primary_arena()
       2. command_line::parse()
       3. config_table::load()
       4. register_log_sinks()
       5. renderer_backend::load_backend()        (optional)
       6. bind_primary_scripting_environment()
       7. bind_environment_scripts()
       8. other_main(cmd, config)                 (user driver)
       9. renderer_backend::unload_backend()      (optional)
      10. cleanup_scripting_environment()
      11. shutdown_subsystems()
```

### Step-by-step Details

#### Arena Initialization

`subsystem<arena>::get()` ensures that the primary arena storage is constructed. Failure to initialize here is fatal; the runtime asserts in debug builds.

#### Command Line Parsing

`core/command_line` recognizes configuration file overrides, working directory hints, verbosity flags, and diagnostics such as `--help`. Working directory changes are applied before any file IO.

#### Configuration Loading

`config_table::load` reads the requested file (typically TOML or JSON) into a typed structure. Missing files degrade gracefully with warnings; invalid content aborts startup.

#### Logging Setup

`register_log_sinks` configures console and file sinks against the logger subsystem in `other-core`. Log levels are set from the configuration; sinks are created lazily via lambdas bound to the config.

#### Rendering Backend (optional)

If the configuration specifies a backend (`config.rendering_backend`), the renderer subsystem loads API-specific modules and initializes window state. Headless deployments skip this step entirely.

#### Scripting Environment Binding

`bind_primary_scripting_environment` initializes the scripting subsystem and loads the primary .NET assembly (`OtherCs.dll` by default). `bind_environment_scripts` registers native exports into the managed host (see `other-scripting/scripting_environment.cpp`).

#### Driver Execution

Control transfers to the application-specific `other_main(const command_line&, const config_table&)`. Drivers are responsible for their own loop and may interact with any initialized subsystem.

#### Tear-down

- Rendering backend unloads (if loaded), releasing GPU resources and window handles.
- Scripting environment unloads assemblies and clears managed references.
- `shutdown_subsystems` iterates known subsystems in dependency order: scripting environment, type database, renderer backend, arena, logger.

## Customizing Startup

### Providing a Custom `main`

If `OTHER_DISABLE_MAIN` is defined on a target, `other::entry` can be called manually after performing custom setup. The standard teardown sequence must still be respected to avoid leaks.
This can be extremely complicated and is not recommended unless your project has to load application specific custom subsytems.

```cpp
#define OTHER_DISABLE_MAIN
#include "other.hpp"

int main(int argc, char** argv) {
  other::initialize_other_environment(argc, argv);
  // Custom work ...
  other::shutdown_other_environment();
  return 0;
}
```

### Adding New Subsystems

1. Define a `subsystem_description<T>` via `OTHER_SUBSYSTEM(T)`.
2. Ensure initialization occurs before use (either implicitly through `subsystem<T>::get()` or explicitly via `subsystem<T>::initialize()`).
3. Extend `shutdown_subsystems` so that teardown order remains deterministic.
4. Expose configuration hooks through `config_table` where appropriate.

### Integrating Alternative Rendering Paths

- Extend `renderer_backend::load_backend` to recognize the new backend string.
- Register backend-specific factories that create swap chains, pipelines, and renderers.
- Track lifetime so that `unload_backend` can dispose the backend cleanly when shutting down.

## Error Handling Strategy

- **Command line / config errors**: return failure codes early, optionally after printing usage or help.
- **Runtime exceptions**: caught around the `other_main` invocation. Debug builds assert, release builds log and convert to `other::FAILURE`.
- **Subsystem failures**: rely on exceptions or error codes; ensure teardown can accommodate partially initialized subsystems.

## Profiling Hooks

`PROFILE_SECTION` macros bracket both `other::entry` and the `other_main` invocation, enabling trace capture via Tracy when compiled with profiling flags (`TRACY_ENABLE`). Additional profiling sections can be added inside drivers without modifying the core pipeline.

## Future Work

- Document window/message pump behaviour for each renderer backend.
- Describe async interactions between networking and scripting subsystems during driver execution.
- Capture teardown ordering constraints formally (dependency graph).
