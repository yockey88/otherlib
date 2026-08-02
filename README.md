# Other Environment

## A Game Development Framework for C++

### Other Environment Architecture

Key concepts of the Other Environment are defined here:

- Module: any of the libraries on which `otherlib.lib` is built (`other-core`, `other-network`, `other-renderer`, etc...)
- Other Driver: the core class of `otherlib.lib` that loads a configuration and possibly a project file and is the main application object.
  - Drivers can be any type of program and custom drivers can load their own subsystem profiles to, for example, leave out rendering or physics for a command line application
- Other Project: a project defined in a `.toml` file (separate from the driver configuration `.toml` file) that specifies a collection of scenes/assets/plugins.
- Other Application: a driver with a project.

Below is an incredibly rough diagram of how an Other Application works

      other-core ---------------------------------------------------|
                    |             |              |                  |
                other-network  other-physics  other-scripting  other-renderer
                    |             |              |                  |
                    |             |              |                  |
                    |             |----|    |----|                  |
                    |                  |    |                       |
                    |----------------| |    | |---------------------|
                                     | |    | |
                                     |---|----|
                                         |
                                     other-ui
                                         |
                                    other-scene
                                         |
                                     otherlib
                                         |
                                |------------------------------|
                                |                              |
                                |                              |
                      (driver-author utils)--------------------|
                                |                              |
            otherlib-driver --------- otherlib-main     User Driver (Other Driver)
        (for dynamic drivers)     (for static drivers)         |  
                                                               |        Other Project (.toml)
                                                               |                 |
                                                               |-----------------|
                                        |----------------------|
                                   Other Application

### Dev Branch Status

[![Dev Stability Assurance](https://github.com/yockey88/otherlib/actions/workflows/dev-stability-check.yml/badge.svg)](https://github.com/yockey88/otherlib/actions/workflows/dev-stability-check.yml)

### Getting The Other Environment

There are two ways to get the environment:

1. **Installer**: download `OtherEnvironment-<version>-windows-x64.exe` (or the SDK zip) from the
   [releases page](https://github.com/yockey88/otherlib/releases) and run it. It installs the SDK to
   `C:\OtherEnvironment` by default and can put `oecli` on your PATH.
2. **Build from source**: see below.

Everything is driven through `oecli`, the Other Environment CLI. Run `oecli help` for the tool list
and `oecli help <tool>` for per-tool usage.

### Building From Source

Prerequisites: Visual Studio 2022 (C++ and C# workloads), CMake 4.x, Python 3.

`oecli` is itself a build artifact, so the repo ships `cli.py`, a small bootstrap wrapper: it builds
`oecli` the first time and then forwards every command to it verbatim. Development therefore uses the
exact same tool users get with a release.

```bash
python cli.py build -c Release
```

That first run configures cmake, builds `oecli`, then builds the full environment and stages the
runtime DLLs next to every application. Afterwards `python cli.py <args>` and
`build/other-cli/<config>/oecli.exe <args>` are interchangeable.

Configurations available:

- `Debug`: unoptimized build with debug symbols (the default everywhere)
- `Release`: optimized build with no debug symbols
- `ProfileD`: unoptimized build with debug symbols and Tracy profiler built in
- `Profile`: optimized build with no debug symbols and Tracy profiler built in

### Common Commands

| Command | What it does |
| --- | --- |
| `oecli create <name>` | scaffold a new project (project file, projectrc, starter scene, C# project) |
| `oecli open <project>` | open a project in the editor |
| `oecli run [editor\|server\|scratch]` | launch a source-tree driver (editor is the default) |
| `oecli scene <compile\|decompile\|info>` | convert and inspect scene documents (`.oscn`/`.oscnb`) |
| `oecli build [--tests] [-c <config>]` | build the environment from source |
| `oecli test [-f <filter>] [-c <config>]` | run the unit test suites (gtest) |
| `oecli test --soak` | run the soak harness and validate its report |
| `oecli install [--prefix <path>]` | install the built SDK (defaults to `C:\OtherEnvironment`) |
| `oecli package [-G "NSIS;ZIP"]` | package the SDK into a zip / windows installer (NSIS required for the installer) |

Every command accepts `--dry-run` (print instead of run), `--env-root <path>` (explicit environment
root), and `oecli help <tool>` documents the rest.

`other-editor` and `other-server` are work-in-progress applications in constant development and are
not yet stable, with the exception of the editor console commands. Type `help` into the editor
command line for a list of commands, and use `--help` on any command for more information.

The drivers can also be launched directly; they resolve resources relative to the repo root:

```bash
./build/other-editor/<config>/other_editor.exe resources/editor-config.toml
```

```bash
./build/other-server/<config>/other_server.exe server-config.toml --cwd other-server
```

### Testing And CI

- Pull requests into `dev` (and pushes to `dev`) build Debug + Release and run the unit test suites.
- Version tags (`v*`) trigger the extensive pipeline: every build configuration is built and run
  through the unit suites plus the soak harness (with the rest of the integration test pipeline
  landing there as it is built), and the release packages (windows installer + SDK zip) are produced
  and attached to the release.

Locally, `python cli.py test` runs the unit suites, `python cli.py test -f '<gtest filter>'` runs a
subset, and `python cli.py test --soak` runs the soak harness.
