/**
 * \file tests/test_main.cpp
 **/
#include <gtest/gtest.h>

#include "core/command_line.hpp"

#include "other.hpp"

/// have to define for linking
exit_code other_main(const command_line& cmd, const config_table& config) { return exit_code::FAILURE; }

namespace other {

  class other_test_environment : public ::testing::Environment {
   public:
    other_test_environment(int argc, char** argv) {
      command_line cmd = command_line::parse(&argc, argv);
    }
    ~other_test_environment() override = default;

    void SetUp() override {
      // if (!cmd.valid) {
      //   FAIL() << "Invalid command line!";
      //   return;
      // }
      // config_table config = config_table::load(cmd.config_file);
      // ASSERT_EQ(config.valid, true) << "Failed to load configuration file: " << cmd.config_file;
      // config.diagnostics.verbose = cmd.diagnostics.verbose;

      subsystem<arena>::get();
      config.core_log_level = spdlog::level::debug;
      register_log_sinks(config);
      CORE_LOG_INFO("Other Environment version {}.{}.{}", OTHERENV_VERSION_MAJOR, OTHERENV_VERSION_MINOR, OTHERENV_VERSION_PATCH);
    }

    void TearDown() override {
      subsystem<logger>::get()->shutdown();
      subsystem<arena>::get()->shutdown();
    }

   private:
    command_line cmd;
    config_table config;
  };

}  // namespace other

int main(int argc, char** argv) {
  /// this is for the CI pipeline which will start running build, but it wil fail to find resources if running there
  other::filepath cwd = std::filesystem::current_path();
  std::println("CWD: {}", cwd.string());
  if (cwd.string().ends_with("build")) {
    std::println("  - adjusting CWD to parent path");
    std::filesystem::current_path(cwd.parent_path());
  }

  ::testing::InitGoogleTest(&argc, argv);
  ::testing::AddGlobalTestEnvironment(new other::other_test_environment(argc, argv));
  return RUN_ALL_TESTS();
}