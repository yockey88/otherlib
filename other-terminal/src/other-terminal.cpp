/**
 * \file other-terminal.cpp
 **/
#include "other.hpp"
#include "terminal/terminal.hpp"

using other::terminal;

int other_main(const command_line& cmdline, const config_table& config) {
  {
    auto* rendering = other::subsystem<other::renderer_backend>::get();
    if (config.rendering_backend.has_value()) {
      rendering->load_backend(config.rendering_backend.value());
    } else {
      CORE_LOG_ERROR("No rendering backend specified in configuration. Cannot proceed.");
      return -1;
    }
  }

  terminal{}.run(cmdline, config);
  other::subsystem<other::renderer_backend>::get()->unload_backend();
  return 0;
}