/**
 * \file plugins/vm_debugger_window.hpp
 **/
#ifndef OTHER_EDITOR_PLUGINS_VM_DEBUGGER_WINDOW_HPP
#define OTHER_EDITOR_PLUGINS_VM_DEBUGGER_WINDOW_HPP

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_memory_editor.h>

#include "driver/driver.hpp"
#include "driver/systems/vm_system.hpp"
#include "plugin/plugin.hpp"
#include "ui/ui_window.hpp"
#include "vm/vm.hpp"

using namespace other;

class vm_debugger_window : public ui_window {
 public:
  vm_debugger_window(event_system* events)
      : ui_window(events, "VM Debugger") {}
  ~vm_debugger_window() override = default;

  void on_render_body() override;

  void render_main_device_controls(other_command_device& device);

 private:
  MemoryEditor mem_editor;

  void render_device(other_command_device& device);
  void render_debugger_home(other_command_device& device);
  void render_program_debugger(other_command_device& device);
};

OTHER_PROVIDES(vm_debugger_window, ui_window, "vm_debugger", OTHER_PARAMS(OTHER_PARAM("name", "vm-debugger")))
OTHER_PLUGIN(tcp_recorder, "0.0.1", "N/A", "records TCP traffic for debugging purposes")

#endif  // OTHER_EDITOR_PLUGINS_VM_DEBUGGER_WINDOW_HPP