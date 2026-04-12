/**
 * \file test/other_test.hpp
 **/
#include <gtest/gtest.h>

#include "core/arena.hpp"
#include "core/command_line.hpp"

#include "physics/physics_environment.hpp"
#include "script/scripting_environment.hpp"

#include "other.hpp"

namespace other {

  inline spdlog::sink_ptr stdout_sink_fn(const config_table& config) {
#ifdef OTHER_ENVIRONMENT_WINDOWS
    return std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
#else
    return std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
#endif
  }

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

      logger* log = subsystem<logger>::get();
      if (log == nullptr) {
        throw std::runtime_error("Logger subsystem is null.");
      }

      log->set_config(&config);
      log->create_logger("other-core-log", spdlog::level::trace);
      // clang-format off
      log_sink sink = {
        1, "console-sink", "%^[%l]%$ %v (%t)",
        (spdlog::level::level_enum)config.core_log_level, stdout_sink_fn,
      };
      log_sink file_sink = {
        2, "file-sink", "[%Y-%m-%d %H:%M:%S -  %t] [%l] %v", spdlog::level::trace,
        [](const config_table& config) -> spdlog::sink_ptr { return std::make_shared<spdlog::sinks::basic_file_sink_mt>(config.core_log_file, true); },
      };
      // clang-format on

      std::string loggers[] = { "other-core-log" };
      log->register_sink(loggers, sink);
      log->register_sink(loggers, file_sink);
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