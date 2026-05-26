/**
 * \file vm/code_generator_000.cpp
 **/
#include "vm/code_generator_000.hpp"

#include "vm/command_files/compiler_error.hpp"
#include "vm/opcode.hpp"

namespace other {

  void code_generator_000::encode_stopdev(const canonical_instruction& instr, lowering_artifact& artifact) {
    OTHER_ASSERT(instr.opcode == canonical_opcode::STOPDEV_OP, "Invalid opcode passed to encode_stopdev");

    emit_instruction(opcode_stop_device(), artifact);
  }

  void code_generator_000::encode_dump(const canonical_instruction& instr, lowering_artifact& artifact) {
    OTHER_ASSERT(instr.opcode == canonical_opcode::DUMP_OP, "Invalid opcode passed to encode_dump");

    if (instr.param[0].kind == operand_kind::INVALID) {
      emit_instruction(opcode_dump_registers(), artifact);
    } else if (instr.param[0].kind == operand_kind::REGISTER_REF && instr.param[1].kind == operand_kind::INVALID) {
      emit_instruction(opcode_dump_register_x(instr.param[0].reg), artifact);
    } else if (instr.param[0].kind == operand_kind::REGISTER_REF && instr.param[1].kind == operand_kind::ADDRESS_U16) {
      emit_instruction(opcode_dump_memory_at(instr.param[0].reg, instr.param[1].val), artifact);
    } else {
      throw ocmd_lowering_error("Invalid operands for dump instruction");
    }
  }

  void code_generator_000::encode_write(const canonical_instruction& instr, lowering_artifact& artifact) {
    OTHER_ASSERT(instr.opcode == canonical_opcode::WRITE_OP, "Invalid opcode passed to encode_write");
    if (instr.param[0].kind != operand_kind::REGISTER_REF) {
      throw ocmd_lowering_error("write first operand must be a register reference");
    }

    if (instr.param[1].kind == operand_kind::ADDRESS_U16) {
      emit_instruction(opcode_write_x_to_memory(instr.param[0].reg, instr.param[1].val), artifact);
    } else if (instr.param[1].kind == operand_kind::DATA_SYMBOL || instr.param[1].kind == operand_kind::CODE_LABEL) {
      add_symbol_fixup(artifact.machine_instructions.size(), instr.param[1].symbol, artifact);
      emit_instruction(opcode_write_x_to_memory(instr.param[0].reg, 0xFFFF), artifact);
    } else {
      throw ocmd_lowering_error("write second operand must be an address or symbol reference");
    }
  }

  void code_generator_000::encode_set(const canonical_instruction& instr, lowering_artifact& artifact) {
    OTHER_ASSERT(instr.opcode == canonical_opcode::SET_OP, "Invalid opcode passed to encode_load");
    if (instr.param[0].kind != operand_kind::REGISTER_REF) {
      throw ocmd_lowering_error("set first operand must be a register reference");
    }

    if (instr.param[1].kind == operand_kind::ADDRESS_U16) {
      emit_instruction(opcode_load_x_from(instr.param[0].reg, instr.param[1].val), artifact);
    } else if (instr.param[1].kind == operand_kind::IMMEDIATE_U16) {
      emit_instruction(opcode_load_x_direct(instr.param[0].reg, instr.param[1].val), artifact);
    } else if (instr.param[1].kind == operand_kind::DATA_SYMBOL || instr.param[1].kind == operand_kind::CODE_LABEL) {
      add_symbol_fixup(artifact.machine_instructions.size(), instr.param[1].symbol, artifact);
      emit_instruction(opcode_load_x_from(instr.param[0].reg, 0xFFFF), artifact);
    } else {
      throw ocmd_lowering_error("set second operand must be an address, immediate, or symbol reference");
    }
  }

  void code_generator_000::encode_cmp(const canonical_instruction& instr, lowering_artifact& artifact) {
    OTHER_ASSERT(instr.opcode == canonical_opcode::CMP_OP, "Invalid opcode passed to encode_cmp");
    if (instr.param[0].kind != operand_kind::REGISTER_REF) {
      throw ocmd_lowering_error("cmp first operand must be a register reference");
    }
    if (instr.param[1].kind != operand_kind::REGISTER_REF) {
      throw ocmd_lowering_error("cmp second operand must be a register reference, immediate, or float literal");
    }
    if (instr.param[2].kind != operand_kind::REGISTER_REF) {
      throw ocmd_lowering_error("cmp param[2]put operand must be a register reference");
    }
    emit_instruction(opcode_compare_x_y_set_z(instr.param[0].reg, instr.param[1].reg, instr.param[2].reg), artifact);
  }

  void code_generator_000::encode_cmpgt(const canonical_instruction& instr, lowering_artifact& artifact) {
    OTHER_ASSERT(instr.opcode == canonical_opcode::CMPGT_OP, "Invalid opcode passed to encode_cmpgt");
    if (instr.param[0].kind != operand_kind::REGISTER_REF) {
      throw ocmd_lowering_error("cmp_gt first operand must be a register reference");
    }
    if (instr.param[1].kind != operand_kind::REGISTER_REF) {
      throw ocmd_lowering_error("cmpgt second operand must be a register reference, immediate, or float literal");
    }
    if (instr.param[2].kind != operand_kind::REGISTER_REF) {
      throw ocmd_lowering_error("cmpgt param[2]put operand must be a register reference");
    }
    emit_instruction(opcode_x_gt_y_set_z(instr.param[0].reg, instr.param[1].reg, instr.param[2].reg), artifact);
  }

  void code_generator_000::encode_cmplt(const canonical_instruction& instr, lowering_artifact& artifact) {
    OTHER_ASSERT(instr.opcode == canonical_opcode::CMPLT_OP, "Invalid opcode passed to encode_cmplt");
    if (instr.param[0].kind != operand_kind::REGISTER_REF) {
      throw ocmd_lowering_error("cmp_lt first operand must be a register reference");
    }
    if (instr.param[1].kind != operand_kind::REGISTER_REF) {
      throw ocmd_lowering_error("cmp_lt second operand must be a register reference, immediate, or float literal");
    }
    if (instr.param[2].kind != operand_kind::REGISTER_REF) {
      throw ocmd_lowering_error("cmp_lt param[2]put operand must be a register reference");
    }
    emit_instruction(opcode_x_lt_y_set_z(instr.param[0].reg, instr.param[1].reg, instr.param[2].reg), artifact);
  }

  void code_generator_000::encode_and(const canonical_instruction& instr, lowering_artifact& artifact) {
    if (instr.opcode != canonical_opcode::AND_OP) {
      throw ocmd_lowering_error("Invalid opcode passed to encode_and");
    }
    if (instr.param[0].kind != operand_kind::REGISTER_REF) {
      throw ocmd_lowering_error("and first operand must be a register reference");
    }
    if (instr.param[1].kind != operand_kind::REGISTER_REF) {
      throw ocmd_lowering_error("and second operand must be a register reference, immediate, or float literal");
    }
    if (instr.param[2].kind != operand_kind::REGISTER_REF) {
      throw ocmd_lowering_error("and param[2]put operand must be a register reference");
    }
    emit_instruction(opcode_x_and_y_set_z(instr.param[0].reg, instr.param[1].reg, instr.param[2].reg), artifact);
  }

  void code_generator_000::encode_or(const canonical_instruction& instr, lowering_artifact& artifact) {
    if (instr.opcode != canonical_opcode::OR_OP) {
      throw ocmd_lowering_error("Invalid opcode passed to encode_or");
    }
    if (instr.param[0].kind != operand_kind::REGISTER_REF) {
      throw ocmd_lowering_error("or first operand must be a register reference");
    }
    if (instr.param[1].kind != operand_kind::REGISTER_REF) {
      throw ocmd_lowering_error("or second operand must be a register reference, immediate, or float literal");
    }
    if (instr.param[2].kind != operand_kind::REGISTER_REF) {
      throw ocmd_lowering_error("or param[2]put operand must be a register reference");
    }
    emit_instruction(opcode_x_or_y_set_z(instr.param[0].reg, instr.param[1].reg, instr.param[2].reg), artifact);
  }

  void code_generator_000::encode_xor(const canonical_instruction& instr, lowering_artifact& artifact) {
    if (instr.opcode != canonical_opcode::XOR_OP) {
      throw ocmd_lowering_error("Invalid opcode passed to encode_xor");
    }
    if (instr.param[0].kind != operand_kind::REGISTER_REF) {
      throw ocmd_lowering_error("xor first operand must be a register reference");
    }
    if (instr.param[1].kind != operand_kind::REGISTER_REF) {
      throw ocmd_lowering_error("xor second operand must be a register reference, immediate, or float literal");
    }
    if (instr.param[2].kind != operand_kind::REGISTER_REF) {
      throw ocmd_lowering_error("xor param[2]put operand must be a register reference");
    }
    emit_instruction(opcode_x_xor_y_set_z(instr.param[0].reg, instr.param[1].reg, instr.param[2].reg), artifact);
  }

  void code_generator_000::encode_lshift(const canonical_instruction& instr, lowering_artifact& artifact) {
    if (instr.opcode != canonical_opcode::LSHIFT_OP) {
      throw ocmd_lowering_error("Invalid opcode passed to encode_lshift");
    }
    if (instr.param[0].kind != operand_kind::REGISTER_REF) {
      throw ocmd_lowering_error("lshift first operand must be a register reference");
    }
    if (instr.param[1].kind != operand_kind::REGISTER_REF) {
      throw ocmd_lowering_error("lshift second operand must be a register reference, immediate, or float literal");
    }
    emit_instruction(opcode_shift_left_x_by_y(instr.param[0].reg, instr.param[1].reg), artifact);
  }

  void code_generator_000::encode_rshift(const canonical_instruction& instr, lowering_artifact& artifact) {
    if (instr.opcode != canonical_opcode::RSHIFT_OP) {
      throw ocmd_lowering_error("Invalid opcode passed to encode_rshift");
    }
    if (instr.param[0].kind != operand_kind::REGISTER_REF) {
      throw ocmd_lowering_error("rshift first operand must be a register reference");
    }
    if (instr.param[1].kind != operand_kind::REGISTER_REF) {
      throw ocmd_lowering_error("rshift second operand must be a register reference, immediate, or float literal");
    }
    emit_instruction(opcode_shift_right_x_by_y(instr.param[0].reg, instr.param[1].reg), artifact);
  }

  void code_generator_000::encode_goto(const canonical_instruction& instr, lowering_artifact& artifact) {
    if (instr.opcode != canonical_opcode::GOTO_OP) {
      throw ocmd_lowering_error("Invalid opcode passed to encode_goto");
    }
    if (instr.param[0].kind != operand_kind::ADDRESS_U16) {
      throw ocmd_lowering_error("goto operand must be an address");
    }
    emit_instruction(opcode_goto(instr.param[0].val), artifact);
  }

  void code_generator_000::encode_je(const canonical_instruction& instr, lowering_artifact& artifact) {
    if (instr.opcode != canonical_opcode::JE_OP) {
      throw ocmd_lowering_error("Invalid opcode passed to encode_je");
    }
    if (instr.param[0].kind != operand_kind::ADDRESS_U16) {
      throw ocmd_lowering_error("je operand must be an address");
    }
    emit_instruction(opcode_jump_if_zero(instr.param[0].val), artifact);
  }

  void code_generator_000::encode_jne(const canonical_instruction& instr, lowering_artifact& artifact) {
    if (instr.opcode != canonical_opcode::JNE_OP) {
      throw ocmd_lowering_error("Invalid opcode passed to encode_jne");
    }
    if (instr.param[0].kind != operand_kind::ADDRESS_U16) {
      throw ocmd_lowering_error("jne operand must be an address");
    }
    emit_instruction(opcode_jump_if_not_zero(instr.param[0].val), artifact);
  }

  void code_generator_000::encode_call(const canonical_instruction& instr, lowering_artifact& artifact) {
    if (instr.opcode != canonical_opcode::CALL_OP) {
      throw ocmd_lowering_error("Invalid opcode passed to encode_call");
    }
    if (instr.param[0].kind == operand_kind::ADDRESS_U16) {
      emit_instruction(opcode_call_at(instr.param[0].val), artifact);
    } else if (instr.param[0].kind == operand_kind::CODE_LABEL || instr.param[0].kind == operand_kind::DATA_SYMBOL) {
      add_symbol_fixup(artifact.machine_instructions.size(), instr.param[0].symbol, artifact);
      emit_instruction(opcode_call_at(0xFFFF), artifact);
    } else {
      throw ocmd_lowering_error("call operand must be an address or symbol reference");
    }
  }

  void code_generator_000::encode_return(const canonical_instruction& instr, lowering_artifact& artifact) {
    if (instr.opcode != canonical_opcode::RET_OP) {
      throw ocmd_lowering_error("Invalid opcode passed to encode_return");
    }
    emit_instruction(opcode_return(), artifact);
  }

  void code_generator_000::encode_add(const canonical_instruction& instr, lowering_artifact& artifact) {
    if (instr.opcode != canonical_opcode::ADD_OP) {
      throw ocmd_lowering_error("Invalid opcode passed to encode_add");
    }
    if (instr.param[0].kind != operand_kind::REGISTER_REF || instr.param[1].kind != operand_kind::REGISTER_REF) {
      throw ocmd_lowering_error("add operands must be register references");
    }
    emit_instruction(opcode_add_x_y_to_x(instr.param[0].reg, instr.param[1].reg), artifact);
  }

  void code_generator_000::encode_sub(const canonical_instruction& instr, lowering_artifact& artifact) {
    if (instr.opcode != canonical_opcode::SUB_OP) {
      throw ocmd_lowering_error("Invalid opcode passed to encode_sub");
    }
    if (instr.param[0].kind != operand_kind::REGISTER_REF || instr.param[1].kind != operand_kind::REGISTER_REF) {
      throw ocmd_lowering_error("sub operands must be register references");
    }
    emit_instruction(opcode_sub_x_y_to_x(instr.param[0].reg, instr.param[1].reg), artifact);
  }

  void code_generator_000::encode_mul(const canonical_instruction& instr, lowering_artifact& artifact) {
    if (instr.opcode != canonical_opcode::MUL_OP) {
      throw ocmd_lowering_error("Invalid opcode passed to encode_mul");
    }
    if (instr.param[0].kind != operand_kind::REGISTER_REF || instr.param[1].kind != operand_kind::REGISTER_REF) {
      throw ocmd_lowering_error("mul operands must be register references");
    }
    emit_instruction(opcode_mul_x_y_to_x(instr.param[0].reg, instr.param[1].reg), artifact);
  }

  void code_generator_000::encode_div(const canonical_instruction& instr, lowering_artifact& artifact) {
    if (instr.opcode != canonical_opcode::DIV_OP) {
      throw ocmd_lowering_error("Invalid opcode passed to encode_div");
    }
    if (instr.param[0].kind != operand_kind::REGISTER_REF || instr.param[1].kind != operand_kind::REGISTER_REF) {
      throw ocmd_lowering_error("div operands must be register references");
    }
    emit_instruction(opcode_div_x_y_to_x(instr.param[0].reg, instr.param[1].reg), artifact);
  }

  void code_generator_000::encode_mod(const canonical_instruction& instr, lowering_artifact& artifact) {
    if (instr.opcode != canonical_opcode::MOD_OP) {
      throw ocmd_lowering_error("Invalid opcode passed to encode_mod");
    }
    if (instr.param[0].kind != operand_kind::REGISTER_REF || instr.param[1].kind != operand_kind::REGISTER_REF) {
      throw ocmd_lowering_error("mod operands must be register references");
    }
    emit_instruction(opcode_mod_x_y_to_x(instr.param[0].reg, instr.param[1].reg), artifact);
  }

  void code_generator_000::encode_loadscn(const canonical_instruction& instr, lowering_artifact& artifact) {
    if (instr.opcode != canonical_opcode::LOADSCN_OP) {
      throw ocmd_lowering_error("Invalid opcode passed to encode_loadscn");
    }
    if (instr.param[0].kind == operand_kind::ADDRESS_U16) {
      emit_instruction(opcode_load_scene_with_id_at(instr.param[0].val), artifact);
    } else if (instr.param[0].kind != operand_kind::DATA_SYMBOL && instr.param[0].kind != operand_kind::CODE_LABEL && instr.param[0].kind != operand_kind::ADDRESS_U16) {
      emit_instruction(opcode_load_scene_with_id_at(instr.param[0].val), artifact);
    } else {
      throw ocmd_lowering_error("loadscn operand must be an address or symbol reference");
    }
  }

  void code_generator_000::encode_playscn(const canonical_instruction& instr, lowering_artifact& artifact) {
    if (instr.opcode != canonical_opcode::PLAYSCN_OP) {
      throw ocmd_lowering_error("Invalid opcode passed to encode_playscn");
    }
    emit_instruction(opcode_play_scene(), artifact);
  }

  void code_generator_000::encode_stopscn(const canonical_instruction& instr, lowering_artifact& artifact) {
    if (instr.opcode != canonical_opcode::STOPSCN_OP) {
      throw ocmd_lowering_error("Invalid opcode passed to encode_stopscn");
    }
    emit_instruction(opcode_stop_scene(), artifact);
  }

}  // namespace other