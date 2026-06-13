/**
 * \file vm/opcode.hpp
 **/
#ifndef OTHERLIB_VM_OPCODE_HPP
#define OTHERLIB_VM_OPCODE_HPP

#include <cstdint>
#include <string>

#include "core/defines.hpp"

namespace other {

  struct other_command_device;

  /**
   * Opcode 32 bit integer format:
   *  - Opcodes can have one of a few different forms depending on the category/type, but the general layout is as follows:
   *   |-------------------------------|
   *   | 2 bytes upper | 2 bytes lower |
   *   |-------------------------------|
   *
   *  - The first 4 bits of the first upper byte (C) represent the opcode category (16 possible categories).
   *  - The last 4 bits of the first upper byte (T) represent the opcode type within that category (16 possible types per category).
   *  - the second upper byte (X) is often a register, but it can also be a small constant or half of a large constant or address
   *  - the first lower byte (Y) is often a register, but it can also be a small constant or half of a large constant or address
   *  - the second lower byte (Z) is often a register or small constant, but it can also be half of a large constant or address
   *  - when the bytes are grouped as an address we refer to it as N (bytes), when they are grouped as a constant we refer to it as K (2 bytes),
   *      and if it is a small constant then it is referred to by A (1 byte)
   *
   *   |-------------------------------|
   *   | C | T |  X/A  |  Y/A  |  Z/A  |
   *   |-------------------------------|
   *
   *   |-------------------------|
   *   | C | T |  X  |   N/K     |
   *   |-------------------------|
   *
   *   |-------------------------|
   *   | C | T |   N/K    |   Z  |
   *   |-------------------------|
   **/

#pragma pack(push, 1)
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
    instruction(int32_t op) : opcode(static_cast<uint32_t>(op)) {}
    instruction(uint32_t op) : opcode(op) {}
    instruction(uint16_t up, uint16_t low) : lower(low), upper(up) {}
    instruction(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4) : bytes{ b1, b2, b3, b4 } {}
    instruction(const instruction& other) : opcode(other.opcode) {}

    uint8_t category_nibble() const { return static_cast<uint8_t>((opcode & kCategoryMask) >> kCategoryShift); }
    uint8_t type_nibble() const { return static_cast<uint8_t>((opcode & kTypeMask) >> kTypeShift); }
  };
  static_assert(sizeof(instruction) == sizeof(uint32_t), "Instruction size must be the same as uint32_t");

  struct syscall {
    syscall() = default;
    syscall(int32_t i) : id(static_cast<uint16_t>(i)) {}
    syscall(uint16_t i) : id(i) {}
    syscall(uint8_t device_id, uint8_t function_id) : function(function_id), device(device_id) {}
    union {
      uint16_t id;
      union {
        struct {
          uint8_t function;
          uint8_t device;
        };
      };
    };
  };
  static_assert(sizeof(syscall) == sizeof(uint16_t), "Syscall size must be the same as uint16_t");
#pragma pack(pop)

  static uint8_t get_category_nibble(uint32_t opcode) {
    return static_cast<uint8_t>((opcode & instruction::kCategoryMask) >> instruction::kCategoryShift);
  }

  static uint8_t get_type_nibble(uint32_t opcode) {
    return static_cast<uint8_t>((opcode & instruction::kTypeMask) >> instruction::kTypeShift);
  }

  enum opcode_categories : uint8_t {
    OPCODE_CATEGORY_DEVICE_CONTROL = 0x0,
    OPCODE_CATEGORY_LOAD_STORE_LOGICAL = 0x1,
    OPCODE_CATEGORY_PROGRAM_FLOW = 0x2,
    OPCODE_CATEGORY_ARITHMETIC = 0x3,
    OPCODE_CATEGORY_SCENE_TABLE = 0x4,

    // ...

    OPCODE_MAX_CATEGORY = 0xF,  // placeholder as we expand to 0xF, this is to make the MAX_NUM_CATEGORIES more readable
  };
  // these are the same because we use a single nibble for both
  /// \todo rename the enum here if we ever expand into the 0xF category
  constexpr inline size_t kMaxNumOpcodeCategories = opcode_categories::OPCODE_MAX_CATEGORY + 1;
  constexpr inline size_t kMaxNumOpcodeTypesPerCategory = kMaxNumOpcodeCategories;

  std::span<const uint8_t> opcode_to_bytes(const instruction& instr);

  std::string opcode_to_simple_string(uint32_t opcode);
  std::string opcode_to_detailed_string(uint32_t opcode);

  uint32_t opcode_read_program_counter(other_command_device* device);
  uint32_t opcode_read_program_counter_and_shift(other_command_device* device);

  uint32_t opcode_set_category(uint32_t opcode, uint8_t category);
  uint32_t opcode_set_type(uint32_t opcode, uint8_t type);
  uint32_t opcode_with_category_and_type(uint8_t category, uint8_t type);
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

  /// Opcode Tables:
  //  0 - device control and debug operations
  //  1 - load/store/logical operations
  //  2 - program flow operations
  //  3 - arithmetic operations
  //  4-F...

  /// Opcode Types:
  //   - there is a maximum of 16 types per category, but not all categories use all 16 types, and some types are shared between categories

  /// 0 table (device control)
  /// 0x00000000 (stopdev)
  uint32_t opcode_stop_device();
  /// 0x01000000 (dump)
  uint32_t opcode_dump_registers();
  /// 0x02xx0000 (dump x)
  uint32_t opcode_dump_register_x(uint8_t x);
  /// 0x03xxnnnn (dump x, n)
  uint32_t opcode_dump_memory_at(uint8_t x, uint16_t n);
  /// 0x04000000 (view_state)
  uint32_t opcode_view_state();

  /// 1 table (load/store/logical)
  /// 10xxnnnn (write x, n)
  uint32_t opcode_write_x_to_memory(uint8_t x, uint16_t n);
  /// 11xxnnnn (set x, n/<label>)
  uint32_t opcode_load_x_from(uint8_t x, uint16_t n);
  /// 12xxkkkk (set x, k)
  uint32_t opcode_load_x_direct(uint8_t x, uint16_t k);
  /// 13xxnnnn (write x, <label>)
  uint32_t opcode_indirect_write_x_to_memory(uint16_t n, uint8_t x);
  /// 14xxyyzz (cmp x, y, z)
  uint32_t opcode_compare_x_y_set_z(uint8_t x, uint8_t y, uint8_t z);
  /// 15xxyyzz (cmpgt x, y, z)
  uint32_t opcode_x_gt_y_set_z(uint8_t x, uint8_t y, uint8_t z);
  /// 16xxyyzz (cmplt x, y, z)
  uint32_t opcode_x_lt_y_set_z(uint8_t x, uint8_t y, uint8_t z);
  /// 17xxyyzz (and x, y, z)
  uint32_t opcode_x_and_y_set_z(uint8_t x, uint8_t y, uint8_t z);
  /// 18xxyyzz (or x, y, z)
  uint32_t opcode_x_or_y_set_z(uint8_t x, uint8_t y, uint8_t z);
  /// 19xxyyzz (xor x, y, z)
  uint32_t opcode_x_xor_y_set_z(uint8_t x, uint8_t y, uint8_t z);
  /// 1Axxyy00 (lshift x, y)
  uint32_t opcode_shift_left_x_by_y(uint8_t x, uint8_t y);
  /// 1Bxxyy00 (rshift x, y)
  uint32_t opcode_shift_right_x_by_y(uint8_t x, uint8_t y);
  /// 1Cxxyy00 (mov x, y)
  uint32_t opcode_move_x_to_y(uint8_t x, uint8_t y);

  /// 2 table (program flow)
  /// 2000nnnn (goto <label>/goto n)
  uint32_t opcode_goto(uint16_t n);
  /// 2100nnnn (je <label>/je n)
  uint32_t opcode_jump_if_zero(uint16_t n);
  /// 2200nnnn (jne <label>/jne n)
  uint32_t opcode_jump_if_not_zero(uint16_t n);
  /// 23xxnnnn (call <label>/call n)
  uint32_t opcode_call_at(uint16_t n);
  /// 24000000 (ret)
  uint32_t opcode_return();
  /// 25xx0000 (ret x)
  uint32_t opcode_return_value_in_x(uint32_t x);
  /// 2600kkkk (syscall x)
  uint32_t opcode_syscall(uint16_t k);

  /// 3 table (arithmetic)
  /// 30xy0000 (add x, y)
  uint32_t opcode_add_x_y_to_x(uint8_t x, uint8_t y);
  /// 31xy0000 (sub x, y)
  uint32_t opcode_sub_x_y_to_x(uint8_t x, uint8_t y);
  /// 32xy0000 (mul x, y)
  uint32_t opcode_mul_x_y_to_x(uint8_t x, uint8_t y);
  /// 33xy0000 (div x, y)
  uint32_t opcode_div_x_y_to_x(uint8_t x, uint8_t y);
  /// 34xy0000 (mod x, y)
  uint32_t opcode_mod_x_y_to_x(uint8_t x, uint8_t y);

  /// 4 table (nop)
  /// 40xxxxxx (nop)
  uint32_t opcode_nop();

}  // namespace other

#endif  // OTHERLIB_VM_OPCODE_HPP