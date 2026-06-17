/**
 * \file vm/register.hpp
 **/
#ifndef OTHERLIB_VM_REGISTER_HPP
#define OTHERLIB_VM_REGISTER_HPP

#include "vm/vm_memory.hpp"

namespace other {

  enum class register_kind : uint8_t {
    GENERAL_PURPOSE,
    RETURN_REGISTER,
    FLAG,
  };

  enum vm_register_idx : uint8_t {
    VM_R0 = 0x00,
    VM_R1 = 0x01,
    VM_R2 = 0x02,
    VM_R3 = 0x03,
    VM_R4 = 0x04,
    VM_R5 = 0x05,
    VM_R6 = 0x06,
    VM_R7 = 0x07,
    VM_R8 = 0x08,
    VM_R9 = 0x09,
    VM_RA = 0x0A,
    VM_RB = 0x0B,
    VM_RC = 0x0C,
    VM_RD = 0x0D,
    VM_RE = 0x0E,
    VM_RF = 0x0F,

    VM_RRETURN = VM_RF,
    VM_RFLAG = 0x10,  // flag register
  };

  struct vm_register {
    constexpr static size_t kRegisterBitSize = 64;  /// 64-bit registers
    constexpr static size_t kNumRegisters = 16;
    constexpr static uint8_t kReturnRegister = VM_RRETURN;
    constexpr static uint8_t kFlagRegister = VM_RFLAG;

    register_kind kind;
    vm_register_idx index;

    register_t<vm_register::kRegisterBitSize> memory;
  };

}  // namespace other

#endif  // OTHERLIB_VM_REGISTER_HPP