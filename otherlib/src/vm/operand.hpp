/**
 * \file vm/operand.hpp
 **/
#ifndef OTHERLIB_VM_OPERAND_HPP
#define OTHERLIB_VM_OPERAND_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "vm/opcode.hpp"
#include "vm/vm_type.hpp"

namespace other {

  enum class operand_kind : uint8_t {
    INVALID = 0,
    REGISTER_REF,
    IMMEDIATE_U16,
    ADDRESS_U16,
    CODE_LABEL,
    DATA_SYMBOL,
    INTEGER_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,
    BLOB_LITERAL,
  };

  // these are the protoype operations that the opcode builder
  //   will use to attempt to parse the parameters into specific opcode formats, this allows compiler to rewrite certain load/write instructions
  //   to handle data-labels/code-labels/weird environment redirection cases, without having to add more opcodes to the vm, and also allows for better error handling during compilation
  enum class canonical_opcode : uint8_t {
    /// 0
    STOPDEV_OP = 0,
    DUMP_OP,
    VIEW_STATE_OP,
    /// 1
    WRITE_OP,
    SET_OP,
    CMP_OP,
    CMPGT_OP,
    CMPLT_OP,
    AND_OP,
    OR_OP,
    XOR_OP,
    LSHIFT_OP,
    RSHIFT_OP,
    MOV_OP,
    /// 2
    GOTO_OP,
    JE_OP,
    JNE_OP,
    CALL_OP,
    RET_OP,
    SYSCALL_OP,
    INVOKE_OP,

    /// 3
    ADD_OP,
    SUB_OP,
    MUL_OP,
    DIV_OP,
    MOD_OP,

    INVALID_OP = 0xFF,
  };

  class opcode_builder;

  struct canonical_operand {
    operand_kind kind = operand_kind::INVALID;
    vm_type type = VM_TYPE_VOID;
    bool indirect = false;

    std::string symbol = "";

    uint8_t reg = 0;
    uint16_t val = 0;
    std::vector<uint8_t> bytes = {};
  };

}  // namespace other

namespace std {

  template <>
  struct formatter<other::canonical_operand> : formatter<std::string> {
    template <typename FormatContext>
    auto format(const other::canonical_operand& operand, FormatContext& ctx) const {
      std::string operand_str;
      switch (operand.kind) {
        case other::operand_kind::REGISTER_REF:
          operand_str = std::format("REG[{:#02x}]", operand.reg);
          break;
        case other::operand_kind::IMMEDIATE_U16:
          operand_str = std::format("IMM[{}]", operand.val);
          break;
        case other::operand_kind::ADDRESS_U16:
          operand_str = std::format("ADDR[{:#04x}]", operand.val);
          break;
        case other::operand_kind::CODE_LABEL:
          operand_str = std::format("CODE_LABEL[{}]", operand.symbol);
          break;
        case other::operand_kind::DATA_SYMBOL:
          operand_str = std::format("DATA_SYMBOL[{}]", operand.symbol);
          break;
        case other::operand_kind::INTEGER_LITERAL:
          operand_str = std::format("INT_LITERAL[{}]", operand.val);
          break;
        case other::operand_kind::FLOAT_LITERAL:
          operand_str = std::format("FLOAT_LITERAL[{} bytes]", operand.bytes.size());
          break;
        case other::operand_kind::STRING_LITERAL:
          operand_str = std::format("STRING_LITERAL[{} bytes]", operand.bytes.size());
          break;
        case other::operand_kind::BLOB_LITERAL:
          operand_str = std::format("BLOB_LITERAL[{} bytes]", operand.bytes.size());
          break;
        default:
          operand_str = "INVALID_OPERAND";
          break;
      }
      return formatter<std::string>::format(operand_str, ctx);
    }
  };

}  // namespace std

#endif  // OTHERLIB_VM_OPERAND_HPP