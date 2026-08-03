# Other Environment CLI

`oecli` manages Other Environment projects and source trees from the command line. It
boots no engine subsystems, so it starts instantly and works outside a running
environment.

```
oecli create my-game --author "Me"     # scaffold a project in ./my-game
oecli open my-game                     # launch it in the editor
oecli run server                       # launch a source-tree driver
oecli build --tests -c Debug           # build the environment from source
oecli test -f 'scene*'                 # run (a subset of) the unit suites
oecli test --soak                      # run the soak harness
oecli install --prefix C:/other-sdk    # cmake --install the built SDK
oecli package -G "NSIS;ZIP"            # produce the installer / sdk zip
oecli help <tool>                      # per-tool usage
```

The developer tools (`run`, `build`, `test`, `install`, `package`) only operate on a
source tree, never an installed SDK. Because `oecli` is itself a build artifact, the
repo root's `cli.py` bootstraps it (configure + build the `oecli` target) and then
forwards every command here verbatim — `python cli.py <args>` is `oecli <args>`.

## Developer cli vs user cli

The build tree produces two front ends from the same tool library, differentiated at
compile time by `OTHER_CLI_DEV_TOOLS`:

- `oecli` — the developer cli, for working on the environment itself. Carries the
  source-tree workflow tools above next to the project workflow. Lives at
  `build/other-cli/<config>/oecli.exe`; this is what `cli.py` builds and forwards to.
- `oecli_user` — the user cli, for making applications/games against the SDK. Still
  named `oecli.exe` but built into `build/other-cli/user/<config>/`. Only the project
  and engine tools (`create`, `open`, `scene`, `model`, `material`) are registered;
  invoking a developer tool prints where it actually lives instead of "unknown tool".

`install`/`package` ship the user cli as the SDK's `oecli.exe` by default; configure
with `-DOTHER_INSTALL_DEV_CLI=ON` to ship the developer cli instead. Build the user
flavor locally with `python cli.py bootstrap --user`.

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
