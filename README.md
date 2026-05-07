# Other Environment

## A Game Development Framework for C++

### Dev Branch Status

[![Dev Stability Assurance - Windows/Release](https://github.com/yockey88/otherlib/actions/workflows/build-and-test-windows-release.yml/badge.svg)](https://github.com/yockey88/otherlib/actions/workflows/build-and-test-windows-release.yml)

[![Dev Stability Assurance - Windows/Debug](https://github.com/yockey88/otherlib/actions/workflows/build-and-test-windows-debug.yml/badge.svg)](https://github.com/yockey88/otherlib/actions/workflows/build-and-test-windows-debug.yml)

### Building And Running

After cloning the repo, simply run the cmake and then use the python scripts to build the projects and copy the DLLs to the correct place:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
python cli.py -b -c=release
```

> CMAKE_BUILD_TYPE will default to debug if not included, the same is true for `cli.py`

After building, you should have the following in the build folder:

- `build/other-editor/${build-type}/other-editor.exe`
- `build/other-server/${build-type}/other-server.exe`
- `build/otherlib/${build-type}/otherlib.lib`
- `build/otherlib/${build-type}/otherlib_main.lib`
- `build/otherlib/${build-type}/otherlib_driver.lib`
- `build/otherlib/${build-type}/otherlib_driver.exe`

`other-editor` and `other-server` are currently work-in-progress applications for personal usage. They can be ran by providing the following command line arguments:

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