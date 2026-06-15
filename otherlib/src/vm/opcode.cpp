/**
 * \file vm/opcode.cpp
 **/
#include "vm/opcode.hpp"

#include "core/logger.hpp"

#include "vm/decompiler.hpp"
#include "vm/other_device.hpp"

namespace other {
  namespace detail {

    static uint32_t set_upper_halfword(uint32_t value, uint16_t upper) {
      return (value & instruction::kLowerHalfwordMask) | (static_cast<uint32_t>(upper) << 16);
    }

    static uint32_t set_lower_halfword(uint32_t value, uint16_t lower) {
      return (value & instruction::kUpperHalfwordMask) | static_cast<uint32_t>(lower);
    }

    static uint32_t opcode_set_value_with_mask(uint32_t dest, uint32_t value, uint32_t mask, uint8_t shift) {
      return (dest & ~mask) | ((value << shift) & mask);
    }

    uint8_t* byte_ptr_at(other_command_device* device) {
      return &device->memory->at(device->pc);
    }

    uint32_t& read_bytes_as_u32(uint8_t* bytes) {
      return *reinterpret_cast<uint32_t*>(std::span(bytes, sizeof(uint32_t)).data());
    }

    uint32_t get_group0_category();
    uint32_t group0_opcode(uint8_t type);
    uint32_t group0_opcode_with_register(uint8_t type, uint8_t reg);
    uint32_t group0_opcode_with_2_registers(uint8_t type, uint8_t reg_x, uint8_t reg_y);
    uint32_t group0_opcode_with_register_and_address(uint8_t type, uint8_t reg, uint16_t addr);

    uint32_t get_group1_category();
    uint32_t group1_opcode(uint8_t type);
    uint32_t group1_opcode_with_register_and_address(uint8_t type, uint8_t reg, uint16_t addr);
    uint32_t group1_opcode_with_register_and_constant(uint8_t type, uint8_t reg, uint16_t k);
    uint32_t group1_opcode_with_2_registers(uint8_t type, uint8_t reg_x, uint8_t reg_y);
    uint32_t group1_opcode_with_3_registers(uint8_t type, uint8_t reg_x, uint8_t reg_y, uint8_t reg_z);

    uint32_t get_group2_category();
    uint32_t group2_opcode(uint8_t type);
    uint32_t group2_opcode_with_register(uint8_t type, uint16_t reg);
    uint32_t group2_opcode_with_address(uint8_t type, uint16_t addr);
    uint32_t group2_opcode_with_register_and_address(uint8_t type, uint16_t reg, uint16_t addr);
    uint32_t group2_opcode_with_constant(uint8_t type, uint16_t k);

    uint32_t get_group3_category();
    uint32_t group3_opcode(uint8_t type);
    uint32_t group3_opcode_with_registers(uint8_t type, uint8_t reg_x, uint8_t reg_y);
    uint32_t group3_opcode_with_register_and_constant(uint8_t type, uint8_t reg, uint16_t k);

    uint32_t get_group4_category();
    uint32_t group4_opcode(uint8_t type, uint8_t reg = 0);
    uint32_t group4_opcode_with_2_registers(uint8_t type, uint8_t reg_x, uint8_t reg_y);
    uint32_t group4_opcode_with_3_registers(uint8_t type, uint8_t reg_x, uint8_t reg_y, uint8_t reg_z);

  }  // namespace detail

  std::span<const uint8_t> opcode_to_bytes(const instruction& instr) {
    return std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&instr), sizeof(instr));
  }

  std::string opcode_to_simple_string(uint32_t opcode) {
    return decompiler::opcode_to_string(opcode);
  }

  std::string opcode_to_detailed_string(uint32_t opcode) {
    return decompiler::opcode_to_detailed_string(opcode);
  }

  uint32_t opcode_read_program_counter(other_command_device* device) {
    return detail::read_bytes_as_u32(detail::byte_ptr_at(device));
  }

  uint32_t opcode_read_program_counter_and_shift(other_command_device* device) {
    uint32_t value = opcode_read_program_counter(device);
    device->pc += other_command_device::kOpCodeSize;
    return value;
  }

  uint32_t opcode_set_category(uint32_t opcode, uint8_t category) {
    return detail::opcode_set_value_with_mask(opcode, category, instruction::kCategoryMask, instruction::kCategoryShift);
  }

  uint32_t opcode_set_type(uint32_t opcode, uint8_t type) {
    return detail::opcode_set_value_with_mask(opcode, type, instruction::kTypeMask, instruction::kTypeShift);
  }

  uint32_t opcode_with_category_and_type(uint8_t category, uint8_t type) {
    return opcode_set_category(opcode_set_type(0, type), category);
  }

  uint32_t opcode_set_x_register(uint32_t opcode, uint8_t x) {
    return detail::opcode_set_value_with_mask(opcode, x, instruction::kXRegisterMask, instruction::kXRegisterShift);
  }

  uint32_t opcode_set_y_register(uint32_t opcode, uint8_t y) {
    return detail::opcode_set_value_with_mask(opcode, y, instruction::kYRegisterMask, instruction::kYRegisterShift);
  }

  uint32_t opcode_set_z_register(uint32_t opcode, uint8_t z) {
    return detail::opcode_set_value_with_mask(opcode, z, instruction::kZRegisterMask, instruction::kZRegisterShift);
  }

  uint32_t opcode_set_x_y_registers(uint32_t opcode, uint8_t x, uint8_t y) {
    return opcode_set_y_register(opcode_set_x_register(opcode, x), y);
  }

  uint32_t opcode_set_x_y_z_registers(uint32_t opcode, uint8_t x, uint8_t y, uint8_t z) {
    return opcode_set_z_register(opcode_set_y_register(opcode_set_x_register(opcode, x), y), z);
  }

  uint32_t opcode_set_k_constant(uint32_t opcode, uint16_t k) {
    return opcode_set_lower(opcode, k);
  }

  uint32_t opcode_set_n_address(uint32_t opcode, uint16_t n) {
    return opcode_set_lower(opcode, n);
  }

  uint32_t opcode_set_x_reg_k_constant(uint32_t opcode, uint8_t x, uint16_t k) {
    return opcode_set_k_constant(opcode_set_x_register(opcode, x), k);
  }

  uint32_t opcode_set_x_reg_n_address(uint32_t opcode, uint8_t x, uint16_t n) {
    return opcode_set_n_address(opcode_set_x_register(opcode, x), n);
  }

  uint32_t opcode_set_upper(uint32_t opcode, uint16_t upper) {
    return detail::set_upper_halfword(opcode, upper);
  }

  uint32_t opcode_set_lower(uint32_t opcode, uint16_t lower) {
    return detail::set_lower_halfword(opcode, lower);
  }

  /// group 0 start ---------------
  uint32_t opcode_stop_device() {
    return 0x00000000;
  }

  uint32_t opcode_dump_registers() {
    return detail::group0_opcode(0x01);
  }

  uint32_t opcode_dump_register_x(uint8_t x) {
    return detail::group0_opcode_with_register(0x02, x);
  }

  uint32_t opcode_dump_memory_at(uint8_t x, uint16_t n) {
    return detail::group0_opcode_with_register_and_address(0x03, x, n);
  }

  uint32_t opcode_view_state() {
    return detail::group0_opcode(0x04);
  }

  uint32_t opcode_nop() {
    return detail::group0_opcode(0x05);
  }

  uint32_t opcode_clear_x_through_y(uint8_t x, uint8_t y) {
    return detail::group0_opcode_with_2_registers(0x06, x, y);
  }

  uint32_t opcode_clear_x(uint8_t x) {
    return opcode_clear_x_through_y(x, x);
  }

  uint32_t opcode_clear() {
    return opcode_clear_x_through_y(0x00, 0x10);
  }
  /// group 0 end -----------------

  /// group 1 start ---------------
  uint32_t opcode_move_x_to_y(uint8_t x, uint8_t y) {
    return detail::group1_opcode_with_2_registers(0x0, x, y);
  }

  uint32_t opcode_write_x_to_memory(uint8_t x, uint16_t n) {
    return detail::group1_opcode_with_register_and_address(0x1, x, n);
  }

  uint32_t opcode_write_x_to_address_in_y(uint8_t x, uint8_t y) {
    return detail::group1_opcode_with_2_registers(0x2, x, y);
  }

  uint32_t opcode_set_x_to_address(uint8_t x, uint16_t n) {
    return detail::group1_opcode_with_register_and_address(0x3, x, n);
  }

  uint32_t opcode_set_x_to_dword_at(uint8_t x, uint16_t n) {
    return detail::group1_opcode_with_register_and_address(0x4, x, n);
  }

  uint32_t opcode_set_x_immediate(uint8_t x, uint8_t k) {
    return detail::group1_opcode_with_register_and_constant(0x5, x, k);
  }
  /// group 1 end -----------------

  /// group 2 start ---------------
  uint32_t opcode_goto(uint16_t n) {
    return detail::group2_opcode_with_address(0x0, n);
  }

  uint32_t opcode_jump_if_zero(uint16_t n) {
    return detail::group2_opcode_with_address(0x1, n);
  }

  uint32_t opcode_jump_if_not_zero(uint16_t n) {
    return detail::group2_opcode_with_address(0x2, n);
  }

  uint32_t opcode_call_at(uint16_t n) {
    return detail::group2_opcode_with_address(0x3, n);
  }

  uint32_t opcode_return() {
    return detail::group2_opcode(0x4);
  }

  uint32_t opcode_return_value_in_x(uint32_t x) {
    return detail::group2_opcode_with_register(0x5, x);
  }

  uint32_t opcode_syscall(uint16_t k) {
    return detail::group2_opcode_with_constant(0x6, k);
  }
  /// group 2 end -----------------

  /// group 3 start ---------------
  uint32_t opcode_add_x_y_to_x(uint8_t x, uint8_t y) {
    return detail::group3_opcode_with_registers(0x0, x, y);
  }

  uint32_t opcode_sub_x_y_to_x(uint8_t x, uint8_t y) {
    return detail::group3_opcode_with_registers(0x1, x, y);
  }

  uint32_t opcode_mul_x_y_to_x(uint8_t x, uint8_t y) {
    return detail::group3_opcode_with_registers(0x2, x, y);
  }

  uint32_t opcode_div_x_y_to_x(uint8_t x, uint8_t y) {
    return detail::group3_opcode_with_registers(0x3, x, y);
  }

  uint32_t opcode_mod_x_y_to_x(uint8_t x, uint8_t y) {
    return detail::group3_opcode_with_registers(0x4, x, y);
  }

  uint32_t opcode_add_x_imm_to_x(uint8_t x, uint16_t k) {
    return detail::group3_opcode_with_register_and_constant(0x5, x, k);
  }

  uint32_t opcode_sub_x_imm_to_x(uint8_t x, uint16_t k) {
    return detail::group3_opcode_with_register_and_constant(0x6, x, k);
  }

  uint32_t opcode_mul_x_imm_to_x(uint8_t x, uint16_t k) {
    return detail::group3_opcode_with_register_and_constant(0x7, x, k);
  }

  uint32_t opcode_div_x_imm_to_x(uint8_t x, uint16_t k) {
    return detail::group3_opcode_with_register_and_constant(0x8, x, k);
  }

  uint32_t opcode_mod_x_imm_to_x(uint8_t x, uint16_t k) {
    return detail::group3_opcode_with_register_and_constant(0x9, x, k);
  }
  /// group 3 end -----------------

  /// group 4 start ---------------
  uint32_t opcode_compare_x_y_set_z(uint8_t x, uint8_t y, uint8_t z) {
    return detail::group4_opcode_with_3_registers(0x0, x, y, z);
  }

  uint32_t opcode_x_gt_y_set_z(uint8_t x, uint8_t y, uint8_t z) {
    return detail::group4_opcode_with_3_registers(0x1, x, y, z);
  }

  uint32_t opcode_x_lt_y_set_z(uint8_t x, uint8_t y, uint8_t z) {
    return detail::group4_opcode_with_3_registers(0x2, x, y, z);
  }

  uint32_t opcode_x_not_set_y(uint8_t x, uint8_t y) {
    return detail::group4_opcode_with_2_registers(0x3, x, y);
  }

  uint32_t opcode_x_and_y_set_z(uint8_t x, uint8_t y, uint8_t z) {
    return detail::group4_opcode_with_3_registers(0x4, x, y, z);
  }

  uint32_t opcode_x_or_y_set_z(uint8_t x, uint8_t y, uint8_t z) {
    return detail::group4_opcode_with_3_registers(0x5, x, y, z);
  }

  uint32_t opcode_x_xor_y_set_z(uint8_t x, uint8_t y, uint8_t z) {
    return detail::group4_opcode_with_3_registers(0x6, x, y, z);
  }

  uint32_t opcode_shift_left_x_by_y_set_z(uint8_t x, uint8_t y, uint8_t z) {
    return detail::group4_opcode_with_3_registers(0x7, x, y, z);
  }

  uint32_t opcode_shift_right_x_by_y_set_z(uint8_t x, uint8_t y, uint8_t z) {
    return detail::group4_opcode_with_3_registers(0x8, x, y, z);
  }
  /// group 4 end -----------------

  /// group 5 start ---------------
  /// group 5 end -----------------
  /// group 6 start ---------------
  /// group 6 end -----------------
  /// group 7 start ---------------
  /// group 7 end -----------------
  /// group 8 start ---------------
  /// group 8 end -----------------
  /// group 9 start ---------------
  /// group 9 end -----------------
  /// group A start ---------------
  /// group A end -----------------
  /// group B start ---------------
  /// group B end -----------------
  /// group C start ---------------
  /// group C end -----------------
  /// group D start ---------------
  /// group D end -----------------
  /// group E start ---------------
  /// group E end -----------------
  /// group F start ---------------
  /// group F end -----------------

  namespace detail {

    uint32_t get_group0_category() {
      return opcode_set_category(0x0, 0x0);
    }

    uint32_t group0_opcode(uint8_t type) {
      return opcode_set_type(get_group0_category(), type);
    }

    uint32_t group0_opcode_with_register(uint8_t type, uint8_t reg) {
      return opcode_set_x_register(opcode_set_type(get_group0_category(), type), reg);
    }

    uint32_t group0_opcode_with_2_registers(uint8_t type, uint8_t reg_x, uint8_t reg_y) {
      return opcode_set_y_register(opcode_set_x_register(opcode_set_type(get_group0_category(), type), reg_x), reg_y);
    }

    uint32_t group0_opcode_with_register_and_address(uint8_t type, uint8_t reg, uint16_t addr) {
      return opcode_set_lower(opcode_set_x_register(opcode_set_type(get_group0_category(), type), reg), addr);
    }

    uint32_t get_group1_category() {
      return opcode_set_category(0x0, 0x1);
    }

    uint32_t group1_opcode(uint8_t type) {
      return opcode_set_type(get_group1_category(), type);
    }

    uint32_t group1_opcode_with_register_and_address(uint8_t type, uint8_t reg, uint16_t addr) {
      return opcode_set_lower(opcode_set_x_register(group1_opcode(type), reg), addr);
    }

    uint32_t group1_opcode_with_register_and_constant(uint8_t type, uint8_t reg, uint16_t k) {
      return opcode_set_k_constant(opcode_set_x_register(group1_opcode(type), reg), k);
    }

    uint32_t group1_opcode_with_2_registers(uint8_t type, uint8_t reg_x, uint8_t reg_y) {
      uint32_t opcode = opcode_set_x_register(group1_opcode(type), reg_x);
      return opcode_set_y_register(opcode, reg_y);
    }

    uint32_t group1_opcode_with_3_registers(uint8_t type, uint8_t reg_x, uint8_t reg_y, uint8_t reg_z) {
      return opcode_set_z_register(opcode_set_y_register(opcode_set_x_register(group1_opcode(type), reg_x), reg_y), reg_z);
    }

    uint32_t get_group2_category() {
      return opcode_set_category(0x0, 0x2);
    }

    uint32_t group2_opcode(uint8_t type) {
      return opcode_set_type(get_group2_category(), type);
    }

    uint32_t group2_opcode_with_register(uint8_t type, uint16_t reg) {
      return opcode_set_x_register(opcode_set_type(get_group2_category(), type), reg);
    }

    uint32_t group2_opcode_with_constant(uint8_t type, uint16_t k) {
      return opcode_set_k_constant(opcode_set_type(get_group2_category(), type), k);
    }

    uint32_t group2_opcode_with_address(uint8_t type, uint16_t addr) {
      return opcode_set_lower(opcode_set_type(get_group2_category(), type), addr);
    }

    uint32_t group2_opcode_with_register_and_address(uint8_t type, uint16_t reg, uint16_t addr) {
      return opcode_set_lower(opcode_set_x_register(opcode_set_type(get_group2_category(), type), reg), addr);
    }

    uint32_t get_group3_category() {
      return opcode_set_category(0x0, 0x3);
    }

    uint32_t group3_opcode(uint8_t type) {
      return opcode_set_type(get_group3_category(), type);
    }

    uint32_t group3_opcode_with_registers(uint8_t type, uint8_t reg_x, uint8_t reg_y) {
      return opcode_set_y_register(opcode_set_x_register(opcode_set_type(get_group3_category(), type), reg_x), reg_y);
    }

    uint32_t group3_opcode_with_register_and_constant(uint8_t type, uint8_t reg, uint16_t k) {
      return opcode_set_k_constant(opcode_set_x_register(opcode_set_type(get_group3_category(), type), reg), k);
    }

    uint32_t get_group4_category() {
      return opcode_set_category(0x0, 0x4);
    }

    uint32_t group4_opcode(uint8_t type, uint8_t reg) {
      return opcode_set_x_register(opcode_set_type(get_group4_category(), type), reg);
    }

    uint32_t group4_opcode_with_2_registers(uint8_t type, uint8_t reg_x, uint8_t reg_y) {
      return opcode_set_y_register(opcode_set_x_register(opcode_set_type(get_group4_category(), type), reg_x), reg_y);
    }

    uint32_t group4_opcode_with_3_registers(uint8_t type, uint8_t reg_x, uint8_t reg_y, uint8_t reg_z) {
      return opcode_set_z_register(opcode_set_y_register(opcode_set_x_register(opcode_set_type(get_group4_category(), type), reg_x), reg_y), reg_z);
    }

  }  // namespace detail
}  // namespace other