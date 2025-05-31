/**
 * \file tests/test_main.cpp
 **/
#include <gtest/gtest.h>

#include "core/command_line.hpp"

#include "other.hpp"

int other_main(const command_line& cmd, const config_table& config) { return -1; }

namespace other {

  class other_test_environment : public ::testing::Environment {
   public:
    other_test_environment(int argc, char** argv) {
      command_line cmd = command_line::parse(&argc, argv);
    }
    ~other_test_environment() override = default;

    void SetUp() override {
      register_log_sinks(config);
      initialize_primary_arena();
      CORE_LOG_INFO("Other Environment version {}.{}.{}", OTHERENV_VERSION_MAJOR, OTHERENV_VERSION_MINOR, OTHERENV_VERSION_PATCH);
      // if (!cmd.valid) {
      //   FAIL() << "Invalid command line!";
      //   return;
      // }

      // config_table config = config_table::load(cmd.config_file);
      // ASSERT_EQ(config.valid, true) << "Failed to load configuration file: " << cmd.config_file;
      // config.diagnostics.verbose = cmd.diagnostics.verbose;
    }

    void TearDown() override {
      shutdown_subsystems();
    }

   private:
    command_line cmd;
    config_table config;
  };

}  // namespace other

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ::testing::AddGlobalTestEnvironment(new other::other_test_environment(argc, argv));
  return RUN_ALL_TESTS();
}