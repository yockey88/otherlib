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

### Building And Running

After cloning the repo, simply run the cmake and then use the python scripts to build the projects and copy the DLLs to the correct place:

```bash
cmake -S . -B build
python cli.py -b -c=<config>
```

Configurations available:

- Debug: unoptimized build with debug symbols
- Release: optimized build with no debug symbols
- ProfileD: unoptimized build with debug symbols and Tracy profiler built in
- Profile: optimized build with no debug symbols and Tracy profile built in

> CMAKE_BUILD_TYPE will default to debug if not included, the same is true for `cli.py`

After building, you should have the following in the build folder:

- `build/other-editor/${build-type}/other-editor.exe`
- `build/other-server/${build-type}/other-server.exe`
- `build/otherlib/${build-type}/otherlib.lib`
- `build/otherlib/${build-type}/otherlib_main.lib`
- `build/otherlib/${build-type}/otherlib_driver.lib`
- `build/otherlib/${build-type}/otherlib_driver.exe`

`other-editor` and `other-server` are currently work-in-progress applications for personal usage, this means they are in constant development (as of now) are not stable. As of right now the exception to this is the console commands in the editor, those should all work. The editor and server can be ran by providing the following command line arguments:

To run the editor:

```bash
./build/other-editor/${build-type}/other_editor.exe resources/editor-config.toml
```

Type  `help` into the editor command line for a list of commands. Use the `--help` on any command for more information.

To run the server:

```bash
./build/other-server/${build-type}/other_server.exe server-config.toml --cwd other-server
```

> the server will not work if not running from the other-server directory of the repo
