/**
 * \file editor-dev/main.cpp
 **/
#include "editor.hpp"
#include "other.hpp"

other::exit_code other_main(const other::command_line& cmd, const other::config_table& config) {
  PROFILE_SECTION("simulation--other_main");
  other::driver* editor = create_driver(&config);
  if (!editor) {
    CORE_LOG_ERROR("Failed to create simulation driver");
    return other::exit_code::FAILURE;
  }

  editor->initialize();
  editor->run();
  editor->shutdown();

  destroy_driver(editor);
  return other::exit_code::SUCCESS;
}