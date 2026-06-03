/**
 * \file plugins/vm_debugger_window.cpp
 **/
#include "vm_debugger_window.hpp"

void vm_debugger_window::on_render_body() {
  driver& drv = get_driver();
  vm_system& vm_sys = drv.get_kernel().get_core_system<vm_system>();

  other_command_device& device = vm_sys.get_device();
  render_main_device_controls(device);
}

void vm_debugger_window::render_main_device_controls(other_command_device& device) {
  if (device.memory == nullptr) {
    scoped_color red_color(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
    ImGui::Text("VM device memory not initialized.");
    return;
  }
  if (device.control_table == nullptr) {
    scoped_color red_color(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
    ImGui::Text("VM device control table not loaded.");
    return;
  }

  natural_t pc = static_cast<natural_t>(device.pc);
  natural_t sp = static_cast<natural_t>(device.sp);
  natural_t load_cursor = static_cast<natural_t>(device.program_load_cursor);

  ImGui::Text("PC: 0x%016llX", pc);
  ImGui::Text("SP: 0x%016llX", sp);
  ImGui::Separator();
  ImGui::Text("[Device State : %s]", device.stopped ? "Stopped" : "Running");
  ImGui::Text("Delay Timer: %u", device.delay_timer);
  ImGui::Text("Sound Timer: %u", device.sound_timer);
  ImGui::Text("Program Load Cursor: 0x%016llX", load_cursor);
  ImGui::Text("Current Instruction: 0x%08X", device.current_instruction.opcode);
  if (device.scene_context != nullptr) {
    ImGui::Text("Scene Context: %p", static_cast<void*>(device.scene_context));
  } else {
    ImGui::Text("Scene Context: None");
  }

  if (ImGui::Button("Step")) {
    vm::step(&device);
  }

  ImGui::Separator();

  for (size_t i = 0; i < other::vm_register::kNumRegisters; ++i) {
    const auto& reg = device.registers[i];
    ImGui::Text("R%02zu: 0x%016llX", i, reg.memory.to_u64());
  }
  ImGui::Separator();

  static MemoryEditor mem_editor;
  // 32 bit bytecode
  mem_editor.Cols = 16;
  mem_editor.DrawContents(device.memory->data, other_command_device::kMemorySize, 0x0000);
}
