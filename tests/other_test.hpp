/**
 * \file test/other_test.hpp
 **/
#include <gtest/gtest.h>

#include "core/arena.hpp"
#include "core/command_line.hpp"

#include "script/scripting_environment.hpp"

#include "other.hpp"

namespace other {

  class other_test_environment : public ::testing::Environment {
   public:
    other_test_environment(int argc, char** argv) {
      cmd = command_line::parse(&argc, argv);
    }
    ~other_test_environment() override = default;

    void SetUp() override {
      if (!std::filesystem::exists(cmd.config_file)) {
        std::println(std::cerr, "Configuration file does not exist: '{}'", cmd.config_file);
        ASSERT_TRUE(false);
      } else {
        std::println(std::cout, "Using configuration file: '{}'", cmd.config_file);
      }

      config = config_table::load(cmd.config_file);
      if (!config.valid) {
        std::println(std::cerr, "Configuration file is invalid: '{}'. Might cause test failures", cmd.config_file);
      }
      config.diagnostics.verbose = cmd.diagnostics.verbose;

      std::println(std::cout, "Configuration Table:\n{}", config.dump_table_string());

      subsystem<arena>::get();
      config.core_log_level = spdlog::level::debug;

      register_log_sinks(config);
      CORE_LOG_INFO("Other Environment version {}.{}.{}", OTHERENV_VERSION_MAJOR, OTHERENV_VERSION_MINOR, OTHERENV_VERSION_PATCH);
    }

    void TearDown() override {
      subsystem<logger>::get()->shutdown();
      subsystem<arena>::get()->shutdown();
    }
    config_table config;

   private:
    command_line cmd;
  };

  class other_test : public ::testing::Test {
   public:
    static inline other_test_environment* environment = nullptr;
  };

}  // namespace other