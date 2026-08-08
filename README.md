# Other Environment

A C++ game development framework with C# scripting. Everything is driven through
`oecli` — `oecli help` lists the tools, `oecli help <tool>` documents each one.
Releases follow [VERSIONING.md](VERSIONING.md); the root `VERSION` file is the source of truth.

## Other Environment Architecture

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

[![Dev Stability Assurance](https://github.com/yockey88/otherlib/actions/workflows/dev-stability-check.yml/badge.svg)](https://github.com/yockey88/otherlib/actions/workflows/dev-stability-check.yml)

## Getting It

Download the installer or SDK zip from the
[releases page](https://github.com/yockey88/otherlib/releases), or build from source.

Prerequisites: Visual Studio 2022 (C++ and C# workloads), CMake 4.x, Python 3.

```bash
python cli.py build -c Release
```

`cli.py` builds `oecli` on first run, then forwards every command to it verbatim.
Configs: `Debug` (default), `Release`, and `ProfileD`/`Profile` (Tracy profiler built in).

## Commands

| Command | What it does |
| --- | --- |
| `oecli create <name>` | scaffold a new project |
| `oecli open <project>` | open a project in the editor |
| `oecli run [editor\|server\|scratch]` | launch a source-tree driver (default: editor) |
| `oecli scene <compile\|decompile\|info>` | convert and inspect scene documents (`.oscn`/`.oscnb`) |
| `oecli model` / `oecli anim` | inspect models, extract/inspect animation clips (`.oanim`) |
| `oecli material` | inspect material files (`.omat`) |
| `oecli build [--tests] [-c <config>]` | build the environment from source |
| `oecli test [-f <filter>] [--soak\|--network\|--fuzz\|--stress]` | run the unit suites / harness scenarios |
| `oecli install` / `oecli package` | install the SDK / package the installer + zip |
