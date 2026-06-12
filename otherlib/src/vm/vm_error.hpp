/**
 * \file vm/vm_error.hpp
 **/
#ifndef OTHERLIB_VM_VM_ERROR_HPP
#define OTHERLIB_VM_VM_ERROR_HPP

namespace other {

  /// \note every syscall/hcall clears rflag to VM_OK before executing
  enum vm_error : uint16_t {
    VM_OK = 0x0000,

    /// machine level
    VM_ILLEGAL_INSTRUCTION_CATEGORY = 0x0001,
    VM_ILLEGAL_INSTRUCTION_TYPE = 0x0002,
    VM_STACK_OVERFLOW = 0x0003,
    VM_STACK_UNDERFLOW = 0x0004,
    VM_DIVISION_BY_ZERO = 0x0005,
    VM_INVALID_MEMORY_ACCESS = 0x0006,
    VM_UNALIGNED_MEMORY_ACCESS = 0x0007,
    VM_OUT_OF_MEMORY = 0x0008,

    /// host-call level
    VM_BAD_SYSCALL = 0x0010,
    VM_BAD_FUNCTION = 0x0011,
    VM_BAD_ARGUMENT = 0x0012,
    VM_BAD_IMPORT = 0x0013,
    VM_STALE_HANDLE = 0x0014,
    VM_DEVICE_UNAVAILABLE = 0x0015,
    VM_HANDLE_TABLE_FULL = 0x0016,
    VM_NO_ACTIVE_SCENE = 0x0017,

    /// scheduler level
    VM_WORK_BUDGET_EXCEEDED = 0x0020,
    VM_QUEUE_OVERFLOW = 0x0021,
    VM_CONTEXT_DIRTY = 0x0022,
  };

}  // namespace other

#endif  // OTHERLIB_VM_VM_ERROR_HPP