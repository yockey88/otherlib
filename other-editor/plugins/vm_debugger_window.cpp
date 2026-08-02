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

  if (vm::has_flag(&device, other_command_device::STOPPED)) {
    render_debugger_home(device);
  } else {
    render_program_debugger(device);
  }
}

void vm_debugger_window::render_device(other_command_device& device) {
  for (size_t i = 0; i < other::vm_register::kNumRegisters; ++i) {
    const auto& reg = device.registers[i];
    ImGui::Text("R%02zu: 0x%016llX", i, reg.memory.to_u64());
  }
  ImGui::Text("R[FLAG] 0x%016llX", device.read_flag_register());
  ImGui::Separator();

  mem_editor.Cols = 8;

  void* memory_data_in_device = device.memory->data;
  mem_editor.DrawContents(memory_data_in_device, other_command_device::kMemorySize, 0x0000);
}

void vm_debugger_window::render_debugger_home(other_command_device& device) {
  {
    scoped_color info_color(ImGuiCol_Text, ImVec4(0.0f, 0.5f, 1.0f, 1.0f));
    ImGui::Text("No Program Executing");
  }

  // file dialog to load program

  render_device(device);
}

void vm_debugger_window::render_program_debugger(other_command_device& device) {
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
  if (device.scene_context != nullptr) {
    ImGui::Text("Scene Context: %p", static_cast<void*>(device.scene_context));
  } else {
    ImGui::Text("Scene Context: None");
  }

  ImGui::Text("Current Instruction: 0x%08X", device.current_instruction.opcode);
  if (ImGui::TreeNode("Instruction Details")) {
    std::string opcode_to_string = opcode_to_detailed_string(device.current_instruction.opcode);
    ImGui::TextWrapped("%s", opcode_to_string.c_str());

    /// render instruction specific break down showing current memory/registers and result after execution

    ImGui::TreePop();
  }

  if (ImGui::Button("Step")) {
    vm::step(&device);
  }

  ImGui::Separator();

  render_device(device);
}
