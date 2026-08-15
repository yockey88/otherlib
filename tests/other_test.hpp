/**
 * \file test/other_test.hpp
 **/
#include <gtest/gtest.h>

#include "core/command_line.hpp"
#include "core/logger_sinks.hpp"
#include "core/version.hpp"
#include "memory/arena.hpp"

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
    }

    config_table config;
    command_line cmd;

   private:
  };

  class other_test : public ::testing::Test {
   public:
    void SetUp() override {
      if (environment == nullptr) {
        FAIL() << "Test environment is not set. Make sure to instantiate other_test_environment in main() and add it to the test framework with AddGlobalTestEnvironment.";
        return;
      }

      std::string test_profile = std::string{ subsystem_profile::kMinimalProfileName };
      if (script_and_physics()) {
        test_profile = std::string{ subsystem_profile::kHeadlessProfileName };
      }

      registry = register_all_subsystems();
      registry.override_subsystem_initialization("logger", std::bind_front(&other_test::initialize_test_logger, this));
      registry.initialize_profile(test_profile, &environment->config);

      if (script_and_physics()) {
        auto* env = subsystem<scripting_environment>::get();
        testing_asm = env->load_dotnet_module(testing_dll.string());
      }
    }

    void TearDown() override {
      if (script_and_physics()) {
        subsystem<scripting_environment>::get()->unload_dotnet_module(testing_asm);
        testing_asm = nullptr;
      }

      registry.shutdown_all(false);
    }
    static inline other_test_environment* environment = nullptr;

   protected:
    virtual bool script_and_physics() const { return false; }

    void initialize_test_logger(const config_table* config) {
      /// shutdown_logger re-inerts on every TearDown; this override replaces
      ///  detail::initialize_logger and must un-inert the same way it does
      subsystem<logger>::inert = false;
      logger* log = subsystem<logger>::get();
      if (log == nullptr) {
        throw std::runtime_error("Logger subsystem is null.");
      }

      std::string test_case = ::testing::UnitTest::GetInstance()->current_test_info()->test_case_name();
      std::string test_name = ::testing::UnitTest::GetInstance()->current_test_info()->name();
      filepath log_file = std::format("{}.{}.log", test_case, test_name);
      log_file = filepath("logs/tests") / log_file;

      log->set_config(config);
      log->create_logger("other-core-log", spdlog::level::trace);
      // clang-format off
      log_sink sink = {
        1, "console-sink", "%^[%l]%$ %v (%t)", (spdlog::level::level_enum)config->core_log_level, 
        stdout_sink_fn,
      };
      log_sink file_sink = {
        2, "file-sink", "[%Y-%m-%d %H:%M:%S -  %t] [%l] %v", spdlog::level::trace,
        [log_file](const config_table& config) -> spdlog::sink_ptr { return std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file.string(), true); },
      };
      // clang-format on

      std::string loggers[] = { "other-core-log" };
      log->register_sink(loggers, &sink);
      log->register_sink(loggers, &file_sink);
    }

   private:
    subsystem_registry registry;

    // filepath other_dll = perform_tag_replacement("build/other-csharp/${build-config}/OtherCs.dll");
    filepath testing_dll = perform_tag_replacement("build/script-testing/${build-config}/DotnetTesting.dll");

    ref<assembly> testing_asm = nullptr;
  };

}  // namespace other
