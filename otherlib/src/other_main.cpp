/**
 * \file other_main.cpp
 **/
#include "other.hpp"

#ifdef OTHER_CLIENT
extern "C" {
/// for building this as the exe in which case a dynamic driver gets loaded and we don't need these
other::driver* create_driver(const other::command_line* cmd, const other::config_table* config) { return nullptr; }
void destroy_driver(other::driver* instance) {}
}
#endif

using other::command_line;
using other::config_table;
using other::exit_code;

exit_code other_main(const command_line& cmd, const config_table& config, const other::subsystem_registry& registry) {
  PROFILE_SECTION("other::main");
  CORE_LOG_INFO("Running Other Runtime [{}]", cmd.config_file);

  auto [driver_instance, driver_name] = other::driver::create(cmd, config);
  if (driver_instance == nullptr) {
    CORE_LOG_ERROR("Failed to create driver instance.");
    return exit_code::FAILURE;
  }

  exit_code res = exit_code::FAILURE;
  auto error_handler = [&](const std::string& error_msg) {
    CORE_LOG_ERROR("!> [FATAL ERROR]: {}", error_msg);
  };

  if (driver_instance != nullptr) {
    CORE_LOG_INFO("Running Other Environment driver '{}'", driver_name);
    driver_instance->initialize(cmd, registry);

    try {
      driver_instance->run();
      res = exit_code::SUCCESS;
    } catch (const other::buffer_parsing_error& e) {
      CORE_LOG_ERROR("A buffer was corrupted. [Arena inspection unimplemented]");
      CORE_LOG_ERROR("Buffer parsing error: {}", e.what());
    } catch (const std::runtime_error& e) {
      error_handler(std::format("A runtime error occurred: {}", e.what()));
    } catch (const std::exception& e) {
      error_handler(std::format("An unexpected error occurred: {}", e.what()));
    } catch (...) {
      error_handler("An unknown error occurred.");
    }

    driver_instance->shutdown();
  }

  other::driver::destroy(driver_name, driver_instance);
  CORE_LOG_INFO("Other Environment driver '{}' has finished unloading.", driver_name);
  return res;
}

int main(int argc, char* argv[]) {
  PROFILE_SECTION("native-main");
  return other::entry(argc, argv);
}