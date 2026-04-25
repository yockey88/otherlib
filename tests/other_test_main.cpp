/**
 * \file tests/test_main.cpp
 **/
#include <gtest/gtest.h>

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