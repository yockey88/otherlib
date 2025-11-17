/**
 * \file vm/program.hpp
 **/
#ifndef OTHERLIB_VM_PROGRAM_HPP
#define OTHERLIB_VM_PROGRAM_HPP

#include <cstdint>
#include <stack>
#include <string>
#include <vector>

namespace other {

  struct program {
    struct label {
      std::string name;
      uint16_t address = 0;
      uint16_t compiled_address = 0;
      std::vector<uint8_t> code = {};
    };
    struct goto_label_instruction {
      uint16_t code_offset;
      uint16_t from_address;
      std::string name;

      uint32_t (*get_opcode)(uint16_t) = nullptr;
    };

    std::stack<std::string> labelstack = {};
    label* current_label = nullptr;

    std::vector<label> labels = {};
    std::vector<goto_label_instruction> goto_labels = {};

    // device assumes two opcodes at the start of memory for initial jump to main and stop device after main returns
    constexpr static uint64_t kDeviceProgramStartCodeOffset = 2 * sizeof(uint32_t);
    uint16_t current_offset = 0;

    uint32_t num_instructions = 0;

    program() = default;
    ~program() = default;

    void start_function(const std::string_view name);
    void start_label(const std::string_view label);
    void end_label();

    void call(const std::string_view name);
    void ret();
    void ret_value_in_x(uint8_t x);

    void dump_registers();
    void dump_x(uint8_t x);

    void write_x_to_memory(uint8_t x, uint16_t n);
    void load_x_from(uint8_t x, uint16_t n);
    void load_x_direct(uint8_t x, uint16_t n);
    void indirect_write_to(uint16_t n, uint8_t x);
    void compare_x_y_set_zero(uint8_t x, uint8_t y);

    void goto_addr(uint64_t n);
    void jne_label(const std::string_view label);

    void add_x_y_to_x(uint8_t x, uint8_t y);
    void sub_x_y_to_x(uint8_t x, uint8_t y);
    void mul_x_y_to_x(uint8_t x, uint8_t y);
    void div_x_y_to_x(uint8_t x, uint8_t y);
    void mod_x_y_to_x(uint8_t x, uint8_t y);

    void load_scene_with_id_at(uint16_t n);

    void dump_program() const;
    std::vector<uint8_t> compile_program(const uint64_t start_address);

   private:
    void add_opcode(uint32_t opcode);
  };

}  // namespace other

#endif  // OTHERLIB_VM_PROGRAM_HPP