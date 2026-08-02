# Other Environment CLI

`oecli` manages Other Environment projects from the command line. It boots no engine
subsystems, so it starts instantly and works outside a running environment.

```
oecli create my-game --author "Me"     # scaffold a project in ./my-game
oecli open my-game                     # launch it in the editor
oecli help <tool>                      # per-tool usage
```

`create` generates the project file (`<name>.toml`), a projectrc (`<name>.lua`), a starter
scene, and a C# script project wired against the environment's `OtherCs.dll`. `open`
resolves a project file (directly, or by searching a directory) and launches
`other_editor` from the environment root with `-f <project>`.

The environment root is discovered from `OTHER_ENVIRONMENT_ROOT`, the executable's own
location, or the working directory (walking upward), with `--env-root` as an explicit
override on each tool.

## Using the tools from inside the environment

Everything lives in the `other_cli` library (namespace `other::cli`); the executable is a
thin front end. `otherlib` links `other_cli`, so drivers, plugins, and project code can
invoke any tool in-process:

```cpp
#include "cli/tool_registry.hpp"

other::cli::tool_result res = other::cli::run("create my-game --dir C:/projects");
```

Hosts that need custom output routing or an explicit environment build a
`other::cli::tool_context` (see `cli/tool.hpp`) and call
`default_tool_registry().execute(...)` / `execute_line(...)`. New tools subclass
`other::cli::tool` and register through `tool_registry::add_tool`.
