/**
 * \file vm/program.cpp
 **/
#include "vm/program.hpp"

#include "core/logger.hpp"

#include "vm/decompiler.hpp"
#include "vm/opcode.hpp"

namespace other {

  void program::start_function(const std::string_view name) {
    current_function = &functions.emplace_back(function{ .name = std::string{ name } });
    current_function->address = current_offset;
  }

  void program::end_function() {
    assert(current_function && "Current Function is null!");
    add_opcode(opcode_return());
    current_function = nullptr;
  }

  void program::call(const std::string_view name) {
    assert(current_function && "Current Function is null!");
    calls.emplace_back(call_instruction{
      .code_offset = static_cast<uint16_t>(current_function->code.size()),
      .from_address = current_function->address,
      .name = std::string{ name },
    });
    /// save an opcode (uint32_t) worth of 0xFF bytes as a placeholder for the call instruction
    uint8_t byte = 0xFF;
    for (uint8_t count = 0; count < sizeof(uint32_t); ++count) {
      current_function->code.emplace_back(byte);
      ++current_offset;
    }
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

  void program::goto_addr(uint64_t n) {
    add_opcode(opcode_goto(n));
  }

  void program::goto_if_zero(uint64_t n) {
    add_opcode(opcode_goto_if_zero(n));
  }

  void program::goto_if_x_zero(uint8_t x, uint64_t n) {
    add_opcode(opcode_goto_if_x_zero(x, n));
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

  void program::dump_program() const {
    for (const auto& func : functions) {
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
    for (auto& func : functions) {
      func.compiled_address = compiled_offset;
      compiled_offset += func.code.size();
    }

    for (const auto& call : calls) {
      auto callee_fn_itr = std::ranges::find(functions, call.from_address, &function::address);
      assert(callee_fn_itr != functions.end() && "Invalid function call! Undefined function!");

      std::string name = call.name;
      auto function_to_call = std::ranges::find(functions, name, &function::name);
      assert(function_to_call != functions.end() && "Invalid function call! Undefined function!");

      uint16_t call_address = function_to_call->compiled_address;
      CORE_LOG_DEBUG(" - [from '{}'] call to [{}] at offset [{:#06x}] pointed to [{}] at address [{:#06x}]", callee_fn_itr->name, function_to_call->name, call.code_offset, function_to_call->name, call_address);

      uint16_t addr = function_to_call->compiled_address;
      uint32_t opcode = other::opcode_call_at(addr);
      *reinterpret_cast<uint32_t*>(&callee_fn_itr->code[call.code_offset]) = opcode;
    }

    for (auto& func : functions) {
      result.append_range(func.code);
    }

    CORE_LOG_DEBUG("Compiled program with {} instructions, size = {} bytes", num_instructions + 2u, result.size());
    return result;
  }

  void program::add_opcode(uint32_t opcode) {
    assert(current_function && "Current Function is null!");

    uint8_t* bytes = reinterpret_cast<uint8_t*>(&opcode);
    for (const auto& byte : std::span(bytes, sizeof(uint32_t))) {
      current_function->code.emplace_back(byte);
      current_offset++;
    }
    ++num_instructions;
  }

}  // namespace other
