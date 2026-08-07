/**
 * \file ui/type-bindings/vm_ui.cpp
 */
#include "ui/type-bindings/vm_ui.hpp"

#include <cstdint>

#include <imgui/ImReflect.hpp>
#include <imgui/imgui_memory_editor.h>

#include "core/profiler.hpp"
#include "vm/opcode.hpp"
#include "vm/other_device.hpp"

IMGUI_REFLECT(other::other_command_device, pc, sp, program_load_cursor);

namespace other {
  namespace ui {

    void device_display::on_render_node_body() {
      PROFILE_SECTION("device_display::on_render_node_body");
      {
        auto settings = ImReflect::ImSettings{};
        settings.push_member<&other_command_device::pc>()
          .read_only()
          .as_hex()
          .format("PC: 0x%08x")
          .pop()
          .push_member<&other_command_device::sp>()
          .read_only()
          .as_hex()
          .format("SP: 0x%08x")
          .pop()
          .push_member<&other_command_device::program_load_cursor>()
          .read_only()
          .as_hex()
          .format("PLC: 0x%08x")
          .pop();
        ImReflect::Input("Device", device_ref, settings);
      }

      ImGui::SeparatorText("====[device memory]===");
      {
        uint32_t curr_op = *reinterpret_cast<const uint32_t*>(&device_ref.memory->at(device_ref.pc));
        std::string curr_op_str = opcode_to_detailed_string(curr_op);

        std::stringstream ss;
        ss << std::format("Current Instruction @ {:#08x} :\n{}\n", device_ref.pc, curr_op_str);

        for (natural_t i = 0; i < vm_register::kNumRegisters; ++i) {
          ss << std::format("R{:<2} : [{}]\n", i, device_ref.read_register_as_u64(i));
        }
        std::string curr_regs_str = ss.str();
        ImGui::Text("%s", curr_regs_str.c_str());

        static MemoryEditor mem_edit;
        mem_edit.OptShowOptions = false;
        mem_edit.Cols = 4;
        mem_edit.DrawContents(device_ref.memory->data, device_ref.memory->size());
      }
    }

  }  // namespace ui
}  // namespace other
