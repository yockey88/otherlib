/**
 * \file vm/opcode.hpp
 **/
#ifndef OTHERLIB_VM_OPCODE_HPP
#define OTHERLIB_VM_OPCODE_HPP

#include <cstdint>

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

    enum : uint8_t {
      X_REGISTER_BYTE_IDX = 2,
      Y_REGISTER_BYTE_IDX = 1,
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

  uint32_t opcode_read_program_counter(other_command_device* device);
  uint32_t opcode_read_program_counter_and_shift(other_command_device* device);

  uint32_t opcode_set_category(uint32_t opcode, uint8_t category);
  uint32_t opcode_set_type(uint32_t opcode, uint8_t type);
  uint32_t opcode_set_x_register(uint32_t opcode, uint8_t x);

  uint32_t opcode_set_upper(uint32_t opcode, uint16_t upper);
  uint32_t opcode_set_lower(uint32_t opcode, uint16_t lower);

  /// 0 table (device control)
  /// 0x00000000
  uint32_t opcode_stop_device();
  /// 0x01000000
  uint32_t opcode_dump_registers();
  /// 0x02xx0000
  uint32_t opcode_dump_register_x(uint8_t x);

  /// 1 table (load/store)
  /// 10xxnnnn
  uint32_t opcode_write_x_to_memory(uint8_t x, uint16_t n);
  /// 11xxnnnn
  uint32_t opcode_load_x_from(uint8_t x, uint16_t n);
  /// 12xxnnnn
  uint32_t opcode_load_x_direct(uint8_t x, uint16_t n);
  /// 13xxnnnn
  uint32_t opcode_indirect_write_x_to_memory(uint16_t n, uint8_t x);

  /// 2 table (program flow)
  /// 2000nnnn
  uint32_t opcode_goto(uint64_t n);
  /// 21xxnnnn
  uint32_t opcode_goto_if_zero(uint64_t n);
  /// 22xxnnnn
  uint32_t opcode_goto_if_x_zero(uint32_t x, uint64_t n);
  /// 23xxnnnn
  uint32_t opcode_call_at(uint64_t n);
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

  /// 4 table (exotic calls)
  /// 40xx0000
  uint32_t opcode_push_environment_object(uint8_t x);

}  // namespace other

#endif  // OTHERLIB_VM_OPCODE_HPP