# Other Environment Entry Point

```text
0. main() / WinMain() takes command line arguments (configuration file, etc...) and forwards directly to other::entry(int, char**)
1. register_log_sinks()
2. renderer_backend::load_backend()                                 (optional)
3. bind_primary_scripting_environment(), bind_environment_scripts() (optional)
4. other_main(cmd, config)                                          (user driver)
5. renderer_backend::unload_backend()                               (optional)
6. cleanup_scripting_environment()                                 (optional)
7. shutdown_subsystems()
```

## Customizing Startup

### Configuration file

If the first argument passed to the command line is a valid `.toml` file then the environment will use it to configure itself. Examples
can be found under `resources/`

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
