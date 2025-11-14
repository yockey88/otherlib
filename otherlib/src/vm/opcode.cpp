/**
 * \file vm/opcode.cpp
 **/
#include "vm/opcode.hpp"

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
    uint32_t group0_opcode(uint32_t opcode, uint8_t type);
    uint32_t group0_opcode_with_register(uint32_t opcode, uint8_t type, uint8_t reg);

    uint32_t get_group1_category();
    uint32_t group1_opcode(uint32_t opcode, uint8_t type, uint8_t reg, uint16_t addr);
    uint32_t group1_opcode_with_constant_and_address(uint32_t opcode, uint8_t type, uint8_t context_idx, uint16_t addr);

    uint32_t get_group2_category();
    uint32_t group2_opcode(uint32_t opcode, uint8_t type);
    uint32_t group2_opcode_with_register(uint32_t opcode, uint8_t type, uint16_t reg);
    uint32_t group2_opcode_with_address(uint32_t opcode, uint8_t type, uint16_t addr);
    uint32_t group2_opcode_with_register_and_address(uint32_t opcode, uint8_t type, uint16_t reg, uint16_t addr);

    uint32_t get_group3_category();
    uint32_t group3_opcode(uint32_t opcode, uint8_t type);
    uint32_t group3_opcode_with_registers(uint32_t opcode, uint8_t type, uint8_t reg_x, uint8_t reg_y);

    uint32_t get_group4_category();
    uint32_t group4_opcode(uint32_t opcode, uint8_t type, uint8_t reg);

  }  // namespace detail

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

  uint32_t opcode_set_x_register(uint32_t opcode, uint8_t x) {
    return detail::opcode_set_value_with_mask(opcode, x, instruction::kXRegisterMask, instruction::kXRegisterShift);
  }

  uint32_t opcode_set_y_register(uint32_t opcode, uint8_t y) {
    return detail::opcode_set_value_with_mask(opcode, y, instruction::kYRegisterMask, instruction::kYRegisterShift);
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
    return detail::group0_opcode(0x0, 0x1);
  }

  uint32_t opcode_dump_register_x(uint8_t x) {
    return detail::group0_opcode_with_register(0x0, 0x2, x);
  }
  /// group 0 end -----------------

  /// group 1 start ---------------
  uint32_t opcode_write_x_to_memory(uint8_t x, uint16_t n) {
    return detail::group1_opcode(0x0, 0x0, x, n);
  }

  uint32_t opcode_load_x_from(uint8_t x, uint16_t n) {
    return detail::group1_opcode(0x0, 0x1, x, n);
  }

  uint32_t opcode_load_x_direct(uint8_t x, uint16_t n) {
    return detail::group1_opcode(0x0, 0x2, x, n);
  }

  uint32_t opcode_indirect_write_x_to_memory(uint16_t n, uint8_t x) {
    return detail::group1_opcode(0x0, 0x3, x, n);
  }

  uint32_t opcode_write_x_to_environment_memory(uint8_t x, uint16_t n) {
    return detail::group1_opcode(0x0, 0x4, x, n);
  }

  uint32_t opcode_load_x_from_environment_memory(uint8_t x, uint16_t n) {
    return detail::group1_opcode(0x0, 0x5, x, n);
  }
  /// group 1 end -----------------

  /// group 2 start ---------------
  uint32_t opcode_goto(uint64_t n) {
    return detail::group2_opcode_with_address(0x0, 0x0, n);
  }

  uint32_t opcode_goto_if_zero(uint64_t n) {
    return detail::group2_opcode_with_address(0x0, 0x1, n);
  }

  uint32_t opcode_goto_if_x_zero(uint32_t x, uint64_t n) {
    return detail::group2_opcode_with_register_and_address(0x0, 0x2, x, n);
  }

  uint32_t opcode_call_at(uint64_t n) {
    return detail::group2_opcode_with_address(0x0, 0x3, n);
  }

  uint32_t opcode_return() {
    return detail::group2_opcode(0x0, 0x4);
  }

  uint32_t opcode_return_value_in_x(uint32_t x) {
    return detail::group2_opcode_with_register(0x0, 0x5, x);
  }
  /// group 2 end -----------------

  /// group 3 start ---------------
  uint32_t opcode_add_x_y_to_x(uint8_t x, uint8_t y) {
    return detail::group3_opcode_with_registers(0x0, 0x0, x, y);
  }

  uint32_t opcode_sub_x_y_to_x(uint8_t x, uint8_t y) {
    return detail::group3_opcode_with_registers(0x0, 0x1, x, y);
  }

  uint32_t opcode_mul_x_y_to_x(uint8_t x, uint8_t y) {
    return detail::group3_opcode_with_registers(0x0, 0x2, x, y);
  }

  uint32_t opcode_div_x_y_to_x(uint8_t x, uint8_t y) {
    return detail::group3_opcode_with_registers(0x0, 0x3, x, y);
  }

  uint32_t opcode_mod_x_y_to_x(uint8_t x, uint8_t y) {
    return detail::group3_opcode_with_registers(0x0, 0x4, x, y);
  }
  /// group 3 end -----------------

  /// group 4 start ---------------
  uint32_t opcode_push_environment_object(uint8_t x) {
    return detail::group4_opcode(0x0, 0x0, x);
  }
  /// group 4 end -----------------

  namespace detail {

    uint32_t get_group0_category() {
      return opcode_set_category(0x0, 0x0);
    }

    uint32_t group0_opcode(uint32_t opcode, uint8_t type) {
      return opcode_set_type(get_group0_category(), type);
    }

    uint32_t group0_opcode_with_register(uint32_t opcode, uint8_t type, uint8_t reg) {
      return opcode_set_x_register(opcode_set_type(get_group0_category(), type), reg);
    }

    uint32_t get_group1_category() {
      return opcode_set_category(0x0, 0x1);
    }

    uint32_t group1_opcode(uint32_t opcode, uint8_t type, uint8_t reg, uint16_t addr) {
      return opcode_set_lower(opcode_set_x_register(opcode_set_type(get_group1_category(), type), reg), addr);
    }

    uint32_t group1_opcode_with_constant_and_address(uint32_t opcode, uint8_t type, uint8_t context_idx, uint16_t addr) {
      opcode = opcode_set_type(get_group1_category(), type);
      opcode = opcode_set_value_with_mask(opcode, context_idx, instruction::kXRegisterMask, instruction::kXRegisterShift);
      return opcode_set_lower(opcode, addr);
    }

    uint32_t get_group2_category() {
      return opcode_set_category(0x0, 0x2);
    }

    uint32_t group2_opcode(uint32_t opcode, uint8_t type) {
      return opcode_set_type(get_group2_category(), type);
    }

    uint32_t group2_opcode_with_register(uint32_t opcode, uint8_t type, uint16_t reg) {
      return opcode_set_x_register(opcode_set_type(get_group2_category(), type), reg);
    }

    uint32_t group2_opcode_with_address(uint32_t opcode, uint8_t type, uint16_t addr) {
      return opcode_set_lower(opcode_set_type(get_group2_category(), type), addr);
    }

    uint32_t group2_opcode_with_register_and_address(uint32_t opcode, uint8_t type, uint16_t reg, uint16_t addr) {
      return opcode_set_lower(opcode_set_x_register(opcode_set_type(get_group2_category(), type), reg), addr);
    }

    uint32_t get_group3_category() {
      return opcode_set_category(0x0, 0x3);
    }

    uint32_t group3_opcode(uint32_t opcode, uint8_t type) {
      return opcode_set_type(get_group3_category(), type);
    }

    uint32_t group3_opcode_with_registers(uint32_t opcode, uint8_t type, uint8_t reg_x, uint8_t reg_y) {
      opcode = opcode_set_x_register(opcode_set_type(get_group3_category(), type), reg_x);
      return opcode_set_y_register(opcode, reg_y);
    }

    uint32_t get_group4_category() {
      return opcode_set_category(0x0, 0x4);
    }

    uint32_t group4_opcode(uint32_t opcode, uint8_t type, uint8_t reg) {
      opcode = opcode_set_type(get_group4_category(), type);
      return opcode_set_x_register(opcode, reg);
    }

  }  // namespace detail
}  // namespace other