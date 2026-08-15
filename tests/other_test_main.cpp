/**
 * \file tests/test_main.cpp
 **/
#include <gtest/gtest.h>

#include <iostream>

#include <Windows.h>

#include "core/logger.hpp"
#include "core/profiler_backend.hpp"

#include "audio/audio_environment.hpp"
#include "thread/thread_safety.hpp"

#include "other_test.hpp"

using other::command_line;
using other::config_table;
using other::exit_code;

/// have to define for linking
exit_code other_main(const command_line& cmd, const config_table& config) { return exit_code::FAILURE; }
extern "C" {
other::driver* otherlib_create_driver(const other::command_line* cmd, const other::config_table* config) { return nullptr; }
void otherlib_destroy_driver(other::driver* instance) {}
}

/// gtest's SEH guard only covers faults raised on the test's own thread — a fault on a
///  background thread (net pump, script finalizers) kills the process silently without this
static LONG WINAPI report_unhandled_seh(EXCEPTION_POINTERS* info) {
  const uint32_t code = info != nullptr && info->ExceptionRecord != nullptr ? info->ExceptionRecord->ExceptionCode : 0;
  std::cerr << "Unhandled SEH exception 0x" << std::hex << code << std::dec << "\nstacktrace =\n"
            << OTHER_STACKTRACE << std::endl;
  return EXCEPTION_CONTINUE_SEARCH;
}

int main(int argc, char** argv) {
  other::profiling::initialize_host_backend();
  SetUnhandledExceptionFilter(&report_unhandled_seh);
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
  other::subsystem<other::audio_environment>::inert = false;

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