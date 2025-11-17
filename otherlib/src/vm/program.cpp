/**
 * \file vm/program.cpp
 **/
#include "vm/program.hpp"

#include "core/logger.hpp"

#include "vm/decompiler.hpp"
#include "vm/opcode.hpp"
#include "vm/other_device.hpp"

namespace other {

  void program::start_function(const std::string_view name) {
    start_label(name);
  }

  void program::start_label(const std::string_view label_title) {
    labelstack.push(std::string{ label_title });
    current_label = &labels.emplace_back(label{ .name = std::string{ label_title } });
    current_label->address = current_offset;
  }

  void program::end_label() {
    labelstack.pop();

    if (labelstack.empty()) {
      current_label = nullptr;
    } else {
      auto itr = std::ranges::find_if(labels, [&](const label& lbl) {
        return lbl.name == labelstack.top();
      });
      if (itr != labels.end()) {
        current_label = &(*itr);
      } else {
        current_label = nullptr;
      }
    }
  }

  void program::call(const std::string_view name) {
    OTHER_ASSERT(current_label, "Current Function is null!");
    goto_labels.emplace_back(goto_label_instruction{
      .code_offset = static_cast<uint16_t>(current_label->code.size()),
      .from_address = current_label->address,
      .name = std::string{ name },
      .get_opcode = [](uint16_t addr) { return opcode_call_at(addr); },
    });
    /// save an opcode (uint32_t) worth of 0xFF bytes as a placeholder for the call instruction
    uint8_t byte = 0xFF;
    for (uint8_t count = 0; count < sizeof(uint32_t); ++count) {
      current_label->code.emplace_back(byte);
      ++current_offset;
    }
  }

  void program::ret() {
    OTHER_ASSERT(current_label, "Current Function is null!");
    add_opcode(opcode_return());
    end_label();
  }

  void program::ret_value_in_x(uint8_t x) {
    OTHER_ASSERT(current_label, "Current Function is null!");
    add_opcode(opcode_return_value_in_x(x));
    end_label();
  }

  void program::dump_registers() {
    add_opcode(opcode_dump_registers());
  }

  void program::dump_x(uint8_t x) {
    add_opcode(opcode_dump_register_x(x));
  }

  void program::write_x_to_memory(uint8_t x, uint16_t n) {
    add_opcode(opcode_write_x_to_memory(x, n));
  }

  void program::load_x_from(uint8_t x, uint16_t n) {
    add_opcode(opcode_load_x_from(x, n));
  }

  void program::load_x_direct(uint8_t x, uint16_t n) {
    add_opcode(opcode_load_x_direct(x, n));
  }

  void program::indirect_write_to(uint16_t n, uint8_t x) {
    add_opcode(opcode_indirect_write_x_to_memory(n, x));
  }

  void program::compare_x_y_set_zero(uint8_t x, uint8_t y) {
    add_opcode(opcode_compare_x_y_set_z(x, y, other_command_device::kFlagRegister));
  }

  void program::goto_addr(uint64_t n) {
    add_opcode(opcode_goto(n));
  }

  void program::jne_label(const std::string_view label) {
    OTHER_ASSERT(current_label, "Current Function is null!");
    goto_labels.emplace_back(goto_label_instruction{
      .code_offset = static_cast<uint16_t>(current_label->code.size()),
      .from_address = current_label->address,
      .name = std::string{ label },
      .get_opcode = [](uint16_t addr) { return opcode_jump_if_zero(addr); },
    });
    /// save an opcode (uint32_t) worth of 0xFF bytes as a placeholder for the call instruction
    uint8_t byte = 0xFF;
    for (uint8_t count = 0; count < sizeof(uint32_t); ++count) {
      current_label->code.emplace_back(byte);
      ++current_offset;
    }
  }

  void program::add_x_y_to_x(uint8_t x, uint8_t y) {
    add_opcode(opcode_add_x_y_to_x(x, y));
  }

  void program::sub_x_y_to_x(uint8_t x, uint8_t y) {
    add_opcode(opcode_sub_x_y_to_x(x, y));
  }

  void program::mul_x_y_to_x(uint8_t x, uint8_t y) {
    add_opcode(opcode_mul_x_y_to_x(x, y));
  }

  void program::div_x_y_to_x(uint8_t x, uint8_t y) {
    add_opcode(opcode_div_x_y_to_x(x, y));
  }

  void program::mod_x_y_to_x(uint8_t x, uint8_t y) {
    add_opcode(opcode_mod_x_y_to_x(x, y));
  }

  void program::load_scene_with_id_at(uint16_t n) {
    add_opcode(opcode_load_scene_with_id_at(n));
  }

  void program::dump_program() const {
    for (const auto& func : labels) {
      CORE_LOG_DEBUG("Function [{}] at address [{:#06x}] with {} bytes of code", func.name, func.address, func.code.size());
      decompiler::dump_instructions(func.code);
    }
  }

  std::vector<uint8_t> program::compile_program(const uint64_t start_address) {
    std::vector<uint8_t> result;

    /// call main function (offset will be after this call instruction, then stop device so 2 + 2)
    uint32_t opcode = other::opcode_call_at(start_address + kDeviceProgramStartCodeOffset);
    uint8_t* opcode_bytes = reinterpret_cast<uint8_t*>(&opcode);

    uint32_t stop_opcode = other::opcode_stop_device();
    uint8_t* stop_opcode_bytes = reinterpret_cast<uint8_t*>(&stop_opcode);

    auto opcode_buffer = std::span(opcode_bytes, sizeof(uint32_t));
    result.append_range(opcode_buffer);

    auto stop_opcode_buffer = std::span(stop_opcode_bytes, sizeof(uint32_t));
    result.append_range(stop_opcode_buffer);

    uint16_t compiled_offset = start_address + kDeviceProgramStartCodeOffset;
    for (auto& func : labels) {
      func.compiled_address = compiled_offset;
      compiled_offset += func.code.size();
    }

    for (const auto& call : goto_labels) {
      auto callee_fn_itr = std::ranges::find(labels, call.from_address, &label::address);
      assert(callee_fn_itr != labels.end() && "Invalid function call! Undefined function!");

      std::string name = call.name;
      auto function_to_call = std::ranges::find(labels, name, &label::name);
      assert(function_to_call != labels.end() && "Invalid function call! Undefined function!");

      uint16_t call_address = function_to_call->compiled_address;
      CORE_LOG_DEBUG(" - [from '{}'] call to [{}] at offset [{:#06x}] pointed to [{}] at address [{:#06x}]", callee_fn_itr->name, function_to_call->name, call.code_offset, function_to_call->name, call_address);

      uint16_t addr = function_to_call->compiled_address;
      uint32_t opcode = call.get_opcode(addr);
      *reinterpret_cast<uint32_t*>(&callee_fn_itr->code[call.code_offset]) = opcode;
    }

    for (auto& func : labels) {
      result.append_range(func.code);
    }

    CORE_LOG_DEBUG("Compiled program with {} instructions, size = {} bytes", num_instructions + 2u, result.size());
    return result;
  }

  void program::add_opcode(uint32_t opcode) {
    assert(current_label && "Current Label is null!");

    uint8_t* bytes = reinterpret_cast<uint8_t*>(&opcode);
    for (const auto& byte : std::span(bytes, sizeof(uint32_t))) {
      current_label->code.emplace_back(byte);
      current_offset++;
    }
    ++num_instructions;
  }

}  // namespace other
