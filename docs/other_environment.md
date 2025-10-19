# Using OtherLib in Your Project

## Overview

OtherLib is a game development framework that controls the application entry point. When you link against `otherlib::otherlib`, the library provides the `main()` function for you.

## Important: Do NOT Define main()

**Your application should NEVER define a `main()` function.** The library provides this for you.

Instead, you must implement:

```cpp
other::exit_code other_main(const other::command_line& cmd, const other::config_table& config) {
    // Your application code here
    return other::exit_code::SUCCESS;
}
```

## Basic CMake Setup

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyOtherApp)

# Find the installed OtherLib package
find_package(otherlib REQUIRED)

# Create your executable
add_executable(my_app
    src/main.cpp  # Contains other_main, NOT main!
    src/game.cpp
    src/other_sources.cpp
)

# Link against otherlib - this automatically provides:
# - main() entry point
# - All platform/build configuration macros (OTHER_ENVIRONMENT_WINDOWS, OTHER_API, etc.)
# - All external dependencies (SDL3, assimp, GLAD, etc.)
target_link_libraries(my_app PRIVATE otherlib::otherlib)
```

## Example Application Code

**src/main.cpp:**
```cpp
#include "other.hpp"
#include "core/logger.hpp"

// Implement other_main instead of main
other::exit_code other_main(const other::command_line& cmd, const other::config_table& config) {
    CORE_LOG_INFO("My Other Application Started!");
    
    // Your game/application logic here
    
    CORE_LOG_INFO("Application finished successfully");
    return other::exit_code::SUCCESS;
}

// NO main() function should be defined here!
```

## Using the Convenience Macros (Recommended)

OtherLib provides helper macros for easier setup:

```cmake
find_package(otherlib REQUIRED)

# For static executables (most common)
add_static_driver(my_app
    src/main.cpp
    src/game.cpp
    src/other_sources.cpp
)

# For dynamic libraries/plugins
add_dynamic_driver(my_plugin
    src/plugin.cpp
    src/systems.cpp
)
```

These macros automatically:
- Configure the correct compile definitions
- Link to otherlib
- Set up platform-specific settings
- Enable the OtherLib entry point

## Opting Out of Automatic main() (Advanced)

If you need to provide your own `main()` for special cases (testing, plugins loaded by other frameworks, etc.):

```cmake
add_executable(my_special_app src/custom_main.cpp)

# Disable the automatic main injection
target_compile_definitions(my_special_app PRIVATE OTHER_DISABLE_MAIN)

target_link_libraries(my_special_app PRIVATE otherlib::otherlib)
```

**src/custom_main.cpp:**
```cpp
#define OTHER_DISABLE_MAIN
#include "other.hpp"

// Now you can define your own main
int main(int argc, char* argv[]) {
    // Custom initialization
    
    // You can still call into otherlib if needed
    other::initialize_other_environment(argc, argv);
    
    // Your code
    
    other::shutdown_other_environment();
    return 0;
}
```

## Common Mistakes

### ❌ WRONG - Defining main():
```cpp
#include "other.hpp"

int main(int argc, char* argv[]) {  // ERROR! Multiple definition of main!
    // ...
}
```

### ✅ CORRECT - Implementing other_main():
```cpp
#include "other.hpp"

other::exit_code other_main(const other::command_line& cmd, const other::config_table& config) {
    // Your code here
    return other::exit_code::SUCCESS;
}
```

## Troubleshooting

### "Multiple definition of main" Error

This means you're defining `main()` somewhere in your code. Remember:
- OtherLib provides `main()` automatically
- You should only implement `other_main()`
- Check all your source files for `int main(` or `int WINAPI WinMain(`

### Solution:
1. Remove all `main()` function definitions from your code
2. Implement `other_main()` instead
3. Use the helper macros: `add_static_driver()` or `add_dynamic_driver()`

## Automatic Compile Definitions

When you link to `otherlib::otherlib`, the following macros are automatically defined:

### Platform Macros
- `OTHER_ENVIRONMENT_WINDOWS` (on Windows)
- `OTHER_ENVIRONMENT_UNIX` (on Linux)

### Build Configuration Macros
- `OTHER_ENVIRONMENT_DEBUG` (Debug builds)
- `OTHER_ENVIRONMENT_RELEASE` (Release builds)
- `OTHER_ENVIRONMENT_PROFILE` (Profile builds)
- `OTHER_ENVIRONMENT_PROFILED` (ProfileD builds)

### API Macros
These are defined in `core/defines.hpp` based on platform and application type:
- `OTHER_API` - Function export/import decoration
- `OTHER_CLASS` - Class export/import decoration
- `OTHER_ALIGN(x)` - Alignment specification

### Windows-Specific Macros
- `NOMINMAX` - Prevents min/max macro conflicts
- `WIN32_LEAN_AND_MEAN` - Reduces Windows header bloat
- `_WIN32_WINNT=0x0A00` - Target Windows 10+
- `_USE_MATH_DEFINES` - Enable math constants (M_PI, etc.)
- `_CRT_SECURE_NO_WARNINGS` - Suppress MSVC security warnings
- Various silence macros for deprecation warnings

You don't need to define these yourself - they're handled automatically!

## Need More Help?

- See the example drivers in `development-drivers/` for reference implementations
- Check the API documentation for available subsystems and utilities
- Review `cmake/add_other_driver.cmake` for the implementation of helper macros
