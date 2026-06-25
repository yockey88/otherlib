/**
 * \file scripting/intefaces/rendering_interfaces.cpp
 **/
#include "scripting/interfaces/rendering_interfaces.hpp"

namespace other {

  environment_interface get_ui_window_interface() {
    return {
      .name = "Other.UI.Window",
      .description = "Interface to interact with UI windows, allowing for handling of window events and data.",
      .actions = {
        {
          .name = "OpenWindow",
          .description = "Action to open a new UI window. Args: (window_id [uint64_t], title [string])",
          .script_name = "OnOpenWindow",
          .plugin_name = "on_open_window",
        },
        {
          .name = "CloseWindow",
          .description = "Action to close an existing UI window. Args: (window_id [uint64_t])",
          .script_name = "OnCloseWindow",
          .plugin_name = "on_close_window",
        },
        {
          .name = "Render",
          .description = "Action to render the UI window. Args: (window_id [uint64_t])",
          .script_name = "OnRender",
          .plugin_name = "on_render",
        },
      }
    };
  }

}  // namespace other