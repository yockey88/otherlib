/**
 * \file tests/test_main.cpp
 **/
#include <gtest/gtest.h>

#include "thread/thread_safety.hpp"

#include "other_test.hpp"

using other::command_line;
using other::config_table;
using other::exit_code;

/// have to define for linking
exit_code other_main(const command_line& cmd, const config_table& config) { return exit_code::FAILURE; }
extern "C" {
other::driver* create_driver(const other::command_line* cmd, const other::config_table* config) { return nullptr; }
void destroy_driver(other::driver* instance) {}
}

int main(int argc, char** argv) {
  other::disable_thread_check();

  // activate all subsystems for the tests
  other::subsystem<other::arena>::inert = false;
  other::subsystem<other::logger>::inert = false;
  other::subsystem<other::file_system>::inert = false;
  other::subsystem<other::input_system>::inert = false;
  other::subsystem<other::type_database>::inert = false;
  other::subsystem<other::physics_environment>::inert = false;
  other::subsystem<other::renderer_backend>::inert = false;
  other::subsystem<other::scripting_environment>::inert = false;

  /// this is for the CI pipeline which will start running build, but it wil fail to find resources if running there
  other::filepath cwd = std::filesystem::current_path();
  std::println("CWD: {}", cwd.string());
  if (cwd.string().ends_with("build")) {
    std::println("  - adjusting CWD to parent path");
    std::filesystem::current_path(cwd.parent_path());
  }

  ::testing::InitGoogleTest(&argc, argv);
  other::other_test::environment = new other::other_test_environment(argc, argv);
  ::testing::AddGlobalTestEnvironment(other::other_test::environment);

  return RUN_ALL_TESTS();
}