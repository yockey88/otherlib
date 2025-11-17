/**
 * \file vm/opcode.hpp
 **/
#ifndef OTHERLIB_VM_OPCODE_HPP
#define OTHERLIB_VM_OPCODE_HPP

#include <cstdint>
#include <string>

namespace other {

  struct other_command_device;

  struct instruction {
    constexpr static uint32_t kCategoryMask = 0xF0000000;
    constexpr static uint8_t kCategoryShift = 28;

    constexpr static uint32_t kTypeMask = 0x0F000000;
    constexpr static uint8_t kTypeShift = 24;

    constexpr static uint32_t kUpperHalfwordMask = 0xFFFF0000;
    constexpr static uint32_t kUpperHalfwordShift = sizeof(uint16_t);

    constexpr static uint32_t kLowerHalfwordMask = 0x0000FFFF;
    constexpr static uint32_t kLowerHalfwordShift = 0;

    constexpr static uint32_t kXRegisterMask = 0x00FF0000;
    constexpr static uint8_t kXRegisterShift = 16;

    constexpr static uint32_t kYRegisterMask = 0x0000FF00;
    constexpr static uint8_t kYRegisterShift = 8;

    constexpr static uint32_t kZRegisterMask = 0x000000FF;
    constexpr static uint8_t kZRegisterShift = 0;

    enum : uint8_t {
      X_REGISTER_BYTE_IDX = 2,
      Y_REGISTER_BYTE_IDX = 1,
      Z_REGISTER_BYTE_IDX = 0,
    };

    union {
      uint32_t opcode = 0;
      union {
        struct {
          uint16_t lower;
          uint16_t upper;
        };
        struct {
          uint8_t bytes[sizeof(uint32_t)];
        };
      };
    };

    instruction() = default;
    instruction(int32_t op) : opcode(op) {}
    instruction(uint32_t op) : opcode(op) {}
    instruction(uint16_t up, uint16_t low) : lower(low), upper(up) {}
    instruction(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4) : bytes{ b1, b2, b3, b4 } {}
    instruction(const instruction& other) : opcode(other.opcode) {}

    uint8_t category_nibble() const { return static_cast<uint8_t>((opcode & kCategoryMask) >> kCategoryShift); }
    uint8_t type_nibble() const { return static_cast<uint8_t>((opcode & kTypeMask) >> kTypeShift); }
  };

  static uint8_t get_category_nibble(uint32_t opcode) {
    return static_cast<uint8_t>((opcode & instruction::kCategoryMask) >> instruction::kCategoryShift);
  }

  static uint8_t get_type_nibble(uint32_t opcode) {
    return static_cast<uint8_t>((opcode & instruction::kTypeMask) >> instruction::kTypeShift);
  }

  std::string opcode_to_simple_string(uint32_t opcode);
  std::string opcode_to_detailed_string(uint32_t opcode);

  uint32_t opcode_read_program_counter(other_command_device* device);
  uint32_t opcode_read_program_counter_and_shift(other_command_device* device);

  uint32_t opcode_set_category(uint32_t opcode, uint8_t category);
  uint32_t opcode_set_type(uint32_t opcode, uint8_t type);
  uint32_t opcode_set_x_register(uint32_t opcode, uint8_t x);
  uint32_t opcode_set_y_register(uint32_t opcode, uint8_t y);
  uint32_t opcode_set_z_register(uint32_t opcode, uint8_t z);
  uint32_t opcode_set_x_y_registers(uint32_t opcode, uint8_t x, uint8_t y);
  uint32_t opcode_set_x_y_z_registers(uint32_t opcode, uint8_t x, uint8_t y, uint8_t z);
  uint32_t opcode_set_k_constant(uint32_t opcode, uint16_t k);
  uint32_t opcode_set_n_address(uint32_t opcode, uint16_t n);
  uint32_t opcode_set_x_reg_k_constant(uint32_t opcode, uint8_t x, uint16_t k);
  uint32_t opcode_set_x_reg_n_address(uint32_t opcode, uint8_t x, uint16_t n);

  uint32_t opcode_set_upper(uint32_t opcode, uint16_t upper);
  uint32_t opcode_set_lower(uint32_t opcode, uint16_t lower);

  uint32_t opcode_get_from_category_and_type(uint32_t category_and_type);

  enum opcode_base : uint32_t {
    OPCODE_STOPDEV = 0x00000000,
    OPCODE_DUMP = 0x01000000,
    OPCODE_DUMPX = 0x02000000,

    OPCODE_WRITE_X_TO_MEM = 0x10000000,
    OPCODE_LOAD_X_FROM_MEM = 0x11000000,
    OPCODE_LOAD_X_DIRECT = 0x12000000,
    OPCODE_INDIRECT_WRITE_X_TO_MEM = 0x13000000,
    OPCODE_COMPARE_X_Y_SET_Z = 0x14000000,
    OPCODE_COMPARE_GT_X_Y_SET_Z = 0x15000000,
    OPCODE_COMPARE_LT_X_Y_SET_Z = 0x16000000,
    OPCODE_X_AND_Y_SET_Z = 0x17000000,
    OPCODE_X_OR_Y_SET_Z = 0x18000000,
    OPCODE_X_XOR_Y_SET_Z = 0x19000000,
    OPCODE_SHIFT_LEFT_X_BY_Y = 0x1A000000,
    OPCODE_SHIFT_RIGHT_X_BY_Y = 0x1B000000,

    OPCODE_GOTO = 0x20000000,
    OPCODE_JUMP_IF_ZERO = 0x21000000,
    OPCODE_JUMP_IF_NOT_ZERO = 0x22000000,
    OPCODE_CALL_AT = 0x23000000,
    OPCODE_RETURN = 0x24000000,
    OPCODE_RETURN_VALUE_IN_X = 0x25000000,

    OPCODE_ADD_X_Y_TO_X = 0x30000000,
    OPCODE_SUB_X_Y_TO_X = 0x31000000,
    OPCODE_MUL_X_Y_TO_X = 0x32000000,
    OPCODE_DIV_X_Y_TO_X = 0x33000000,
    OPCODE_MOD_X_Y_TO_X = 0x34000000,

    OPCODE_LOAD_SCENE_WITH_ID_AT = 0x40000000,

    OPCODE_INVALID = 0xFFFFFFFF,
  };

  /// 0 table (device control)
  /// 0x00000000
  uint32_t opcode_stop_device();
  /// 0x01000000
  uint32_t opcode_dump_registers();
  /// 0x02xx0000
  uint32_t opcode_dump_register_x(uint8_t x);

  /// 1 table (load/store/logical)
  /// 10xxnnnn
  uint32_t opcode_write_x_to_memory(uint8_t x, uint16_t n);
  /// 11xxnnnn
  uint32_t opcode_load_x_from(uint8_t x, uint16_t n);
  /// 12xxkkkk
  uint32_t opcode_load_x_direct(uint8_t x, uint16_t k);
  /// 13xxnnnn
  uint32_t opcode_indirect_write_x_to_memory(uint16_t n, uint8_t x);
  /// 14xxyyzz
  uint32_t opcode_compare_x_y_set_z(uint8_t x, uint8_t y, uint8_t z);
  /// 15xxyyzz
  uint32_t opcode_x_gt_y_set_z(uint8_t x, uint8_t y, uint8_t z);
  /// 16xxyyzz
  uint32_t opcode_x_lt_y_set_z(uint8_t x, uint8_t y, uint8_t z);
  /// 17xxyyzz
  uint32_t opcode_x_and_y_set_z(uint8_t x, uint8_t y, uint8_t z);
  /// 18xxyyzz
  uint32_t opcode_x_or_y_set_z(uint8_t x, uint8_t y, uint8_t z);
  /// 19xxyyzz
  uint32_t opcode_x_xor_y_set_z(uint8_t x, uint8_t y, uint8_t z);
  /// 1Axxyy00
  uint32_t opcode_shift_left_x_by_y(uint8_t x, uint8_t y);
  /// 1Bxxyy00
  uint32_t opcode_shift_right_x_by_y(uint8_t x, uint8_t y);

  /// 2 table (program flow)
  /// 2000nnnn
  uint32_t opcode_goto(uint16_t n);
  /// 2100nnnn
  uint32_t opcode_jump_if_zero(uint16_t n);
  /// 2200nnnn
  uint32_t opcode_jump_if_not_zero(uint16_t n);
  /// 23xxnnnn
  uint32_t opcode_call_at(uint16_t n);
  /// 24000000
  uint32_t opcode_return();
  /// 25xx0000
  uint32_t opcode_return_value_in_x(uint32_t x);

  /// 3 table (arithmetic)
  /// 30xy0000
  uint32_t opcode_add_x_y_to_x(uint8_t x, uint8_t y);
  /// 31xy0000
  uint32_t opcode_sub_x_y_to_x(uint8_t x, uint8_t y);
  /// 32xy0000
  uint32_t opcode_mul_x_y_to_x(uint8_t x, uint8_t y);
  /// 33xy0000
  uint32_t opcode_div_x_y_to_x(uint8_t x, uint8_t y);
  /// 34xy0000
  uint32_t opcode_mod_x_y_to_x(uint8_t x, uint8_t y);

  /// 4 table (core scene-control calls)
  /// 4000nnnn
  uint32_t opcode_load_scene_with_id_at(uint16_t n);

}  // namespace other

#endif  // OTHERLIB_VM_OPCODE_HPP