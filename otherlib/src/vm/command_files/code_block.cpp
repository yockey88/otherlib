/**
 * \file vm/command_files/code_block.cpp
 **/
#include "vm/command_files/code_block.hpp"

#include "core/logger.hpp"

#include "vm/opcode.hpp"

#include "token.hpp"

namespace other {
  namespace {

    uint8_t instruction_register_index_from_token(const token& tok) {
      if (tok.type != TOKEN_TYPE_REGISTER) {
        throw std::runtime_error("Token is not a register: " + tok.text);
      }

      if ("r1" == tok.text) return 0x01;
      if ("r2" == tok.text) return 0x02;
      if ("r3" == tok.text) return 0x03;
      if ("r4" == tok.text) return 0x04;
      if ("r5" == tok.text) return 0x05;
      if ("r6" == tok.text) return 0x06;
      if ("r7" == tok.text) return 0x07;
      if ("r8" == tok.text) return 0x08;
      if ("r9" == tok.text) return 0x09;
      if ("ra" == tok.text) return 0x0A;
      if ("rb" == tok.text) return 0x0B;
      if ("rc" == tok.text) return 0x0C;
      if ("rd" == tok.text) return 0x0D;
      if ("re" == tok.text) return 0x0E;
      if ("rf" == tok.text) return 0x0F;
      if ("rflag" == tok.text) return 0x10;

      throw std::runtime_error("Invalid register token: " + tok.text);
    }

  }  // namespace

  raw_instruction::argument raw_instruction::argument::from_token(const token& tok) {
    auto arg = raw_instruction::argument{
      .raw_txt = tok.text,
      .type = tok.type,
    };

    if (tok.type == TOKEN_TYPE_REGISTER) {
      arg.value = instruction_register_index_from_token(tok);
    } else if (tok.type == TOKEN_TYPE_ADDRESS) {
      arg.value = static_cast<uint16_t>(std::stoul(tok.text, nullptr, 16));
    } else if (tok.type == TOKEN_TYPE_INTEGER_LITERAL) {
      arg.value = static_cast<uint16_t>(std::stoul(tok.text, nullptr, 10));
    } else if (tok.type == TOKEN_TYPE_FLOATING_POINT_LITERAL) {
      float fvalue = std::stof(tok.text);
      arg.raw_data.resize(sizeof(float));
      std::memcpy(arg.raw_data.data(), &fvalue, sizeof(float));
    } else if (tok.type == TOKEN_TYPE_STRING_LITERAL) {
      arg.raw_data = std::vector<uint8_t>(tok.text.begin(), tok.text.end());
    } else if (tok.type == TOKEN_TYPE_LABEL) {
      /// mark as unresolved address for now
      arg.value = 0xFFFF;
    }

    return arg;
  }

  uint32_t raw_instruction::get_opcode(uint32_t opcode, const std::vector<argument>& arguments) {
    auto mark_load_opcode_label_arguments = [](uint32_t opcode, const std::vector<argument>& args) -> uint32_t {
      if (args[1].type == TOKEN_TYPE_INTEGER_LITERAL || args[1].type == TOKEN_TYPE_ADDRESS) {
        return opcode_set_x_reg_n_address(opcode, args[0].value.value(), args[1].value.value());
      } else if (args[1].type == TOKEN_TYPE_FLOATING_POINT_LITERAL) {
        OTHER_ASSERT(false, "Floating point literals not yet supported in LOAD_X_DIRECT");
      } else if (args[1].type == TOKEN_TYPE_STRING_LITERAL) {
        OTHER_ASSERT(false, "String literals not yet supported in LOAD_X_DIRECT");
      } else if (args[1].type == TOKEN_TYPE_LABEL) {
        return opcode_set_x_reg_n_address(opcode, args[0].value.value(), 0xFFFF);
      } else {
        OTHER_ASSERT(false, "Unsupported argument type for LOAD_X_DIRECT: {}", args[1].type);
      }
    };

    switch (opcode) {
      case OPCODE_STOPDEV:
        OTHER_ASSERT(arguments.size() == 0, "STOPDEV takes no arguments : arguments.size() = {}", arguments.size());
        return opcode;
      case OPCODE_DUMP:
        OTHER_ASSERT(arguments.size() == 0, "DUMP_REGISTERS takes no arguments : arguments.size() = {}", arguments.size());
        return opcode;
      case OPCODE_DUMPX:
        OTHER_ASSERT(arguments.size() == 1, "DUMP_MEMORY takes 1 argument : arguments.size() = {}", arguments.size());
        return opcode_set_x_register(opcode, arguments[0].value.value());

      case OPCODE_WRITE_X_TO_MEM:
        OTHER_ASSERT(arguments.size() == 2, "WRITE takes 2 arguments : arguments.size() = {}", arguments.size());
        return opcode_set_x_y_registers(opcode, arguments[0].value.value(), arguments[1].value.value());
      case OPCODE_LOAD_X_DIRECT:
        OTHER_ASSERT(arguments.size() == 2, "LOAD takes 2 arguments : arguments.size() = {}", arguments.size());
        return mark_load_opcode_label_arguments(opcode, arguments);
        break;
      case OPCODE_LOAD_X_FROM_MEM:
        OTHER_ASSERT(arguments.size() == 2, "LOAD takes 2 arguments : arguments.size() = {}", arguments.size());
        return mark_load_opcode_label_arguments(opcode, arguments);
      case OPCODE_INDIRECT_WRITE_X_TO_MEM:
        OTHER_ASSERT(arguments.size() == 2, "INDIRECT_WRITE takes 2 arguments : arguments.size() = {}", arguments.size());
        return opcode_set_x_y_registers(opcode, arguments[0].value.value(), arguments[1].value.value());
      case OPCODE_COMPARE_X_Y_SET_Z:
        OTHER_ASSERT(arguments.size() == 3, "COMPARE takes 3 arguments : arguments.size() = {}", arguments.size());
        return opcode_set_x_y_z_registers(opcode, arguments[0].value.value(), arguments[1].value.value(), arguments[2].value.value());
      case OPCODE_COMPARE_GT_X_Y_SET_Z:
        OTHER_ASSERT(arguments.size() == 3, "GT takes 3 arguments : arguments.size() = {}", arguments.size());
        return opcode_set_x_y_z_registers(opcode, arguments[0].value.value(), arguments[1].value.value(), arguments[2].value.value());
      case OPCODE_COMPARE_LT_X_Y_SET_Z:
        OTHER_ASSERT(arguments.size() == 3, "LT takes 3 arguments : arguments.size() = {}", arguments.size());
        return opcode_set_x_y_z_registers(opcode, arguments[0].value.value(), arguments[1].value.value(), arguments[2].value.value());
      case OPCODE_X_AND_Y_SET_Z:
        OTHER_ASSERT(arguments.size() == 3, "AND takes 3 arguments : arguments.size() = {}", arguments.size());
        return opcode_set_x_y_z_registers(opcode, arguments[0].value.value(), arguments[1].value.value(), arguments[2].value.value());
      case OPCODE_X_OR_Y_SET_Z:
        OTHER_ASSERT(arguments.size() == 3, "OR takes 3 arguments : arguments.size() = {}", arguments.size());
        return opcode_set_x_y_z_registers(opcode, arguments[0].value.value(), arguments[1].value.value(), arguments[2].value.value());
      case OPCODE_X_XOR_Y_SET_Z:
        OTHER_ASSERT(arguments.size() == 3, "XOR takes 3 arguments : arguments.size() = {}", arguments.size());
        return opcode_set_x_y_z_registers(opcode, arguments[0].value.value(), arguments[1].value.value(), arguments[2].value.value());
      case OPCODE_SHIFT_LEFT_X_BY_Y:
        OTHER_ASSERT(arguments.size() == 2, "SHIFT_LEFT takes 2 arguments : arguments.size() = {}", arguments.size());
        return opcode_set_x_y_registers(opcode, arguments[0].value.value(), arguments[1].value.value());
      case OPCODE_SHIFT_RIGHT_X_BY_Y:
        OTHER_ASSERT(arguments.size() == 2, "SHIFT_RIGHT takes 2 arguments : arguments.size() = {}", arguments.size());
        return opcode_set_x_y_registers(opcode, arguments[0].value.value(), arguments[1].value.value());

      case OPCODE_GOTO:
        OTHER_ASSERT(arguments.size() == 1, "GOTO takes 1 argument : arguments.size() = {}", arguments.size());
        return opcode_set_n_address(opcode, arguments[0].value.value());
      case OPCODE_JUMP_IF_ZERO:
        OTHER_ASSERT(arguments.size() == 1, "GOTO_IF_ZERO takes 1 argument : arguments.size() = {}", arguments.size());
        return opcode_set_n_address(opcode, arguments[0].value.value());
      case OPCODE_JUMP_IF_NOT_ZERO:
        OTHER_ASSERT(arguments.size() == 1, "GOTO_IF_NOT_ZERO takes 1 argument : arguments.size() = {}", arguments.size());
        return opcode_set_n_address(opcode, arguments[0].value.value());
      case OPCODE_CALL_AT:
        OTHER_ASSERT(arguments.size() == 1, "CALL takes 1 argument : arguments.size() = {}", arguments.size());
        return opcode_set_n_address(opcode, arguments[0].value.value());
      case OPCODE_RETURN:
        OTHER_ASSERT(arguments.size() == 0, "RETURN takes no arguments : arguments.size() = {}", arguments.size());
        return opcode;
      case OPCODE_RETURN_VALUE_IN_X:
        OTHER_ASSERT(arguments.size() == 1, "RETURN_VALUE_IN_X takes 1 argument : arguments.size() = {}", arguments.size());
        return opcode_set_x_register(opcode, arguments[0].value.value());

      case OPCODE_ADD_X_Y_TO_X:
        OTHER_ASSERT(arguments.size() == 2, "ADD takes 2 arguments : arguments.size() = {}", arguments.size());
        return opcode_set_x_y_registers(opcode, arguments[0].value.value(), arguments[1].value.value());
      case OPCODE_SUB_X_Y_TO_X:
        OTHER_ASSERT(arguments.size() == 2, "SUB takes 2 arguments : arguments.size() = {}", arguments.size());
        return opcode_set_x_y_registers(opcode, arguments[0].value.value(), arguments[1].value.value());
      case OPCODE_MUL_X_Y_TO_X:
        OTHER_ASSERT(arguments.size() == 2, "MUL takes 2 arguments : arguments.size() = {}", arguments.size());
        return opcode_set_x_y_registers(opcode, arguments[0].value.value(), arguments[1].value.value());
      case OPCODE_DIV_X_Y_TO_X:
        OTHER_ASSERT(arguments.size() == 2, "DIV takes 2 arguments : arguments.size() = {}", arguments.size());
        return opcode_set_x_y_registers(opcode, arguments[0].value.value(), arguments[1].value.value());
      case OPCODE_MOD_X_Y_TO_X:
        OTHER_ASSERT(arguments.size() == 2, "MOD takes 2 arguments : arguments.size() = {}", arguments.size());
        return opcode_set_x_y_registers(opcode, arguments[0].value.value(), arguments[1].value.value());

      case OPCODE_LOAD_SCENE_WITH_ID_AT:
        OTHER_ASSERT(arguments.size() == 1, "LOAD_SCENE_WITH_ID takes 1 argument : arguments.size() = {}", arguments.size());
        return opcode_set_n_address(opcode, arguments[0].value.value());

      default:
        OTHER_ASSERT(false, "Unhandled opcode in raw_instruction::get_opcode");
        return 0;
    }
  }

  std::vector<token> raw_instruction::get_argument_tokens_for_instruction(const uint32_t category_and_type, const std::vector<token>& arg_tokens) {
    /// a few simple cases
    if (arg_tokens.size() < 3) {
      return arg_tokens;
    } else if (arg_tokens.size() == 3) {
      if (arg_tokens[1].type == TOKEN_TYPE_DOT) {
        token joined_tok;
        joined_tok.type = TOKEN_TYPE_IDENTIFIER;
        joined_tok.text = arg_tokens[0].text + "." + arg_tokens[2].text;
        joined_tok.line_number = arg_tokens[0].line_number;
        joined_tok.column_number = arg_tokens[0].column_number;
        return { joined_tok };
      } else {
        return arg_tokens;
      }
    }

    std::vector<token> conjoined_tokens;

    ///  join any two tokens seperated by a dot into one token (for e.g., struct.field)
    for (size_t i = 1; i < arg_tokens.size(); ++i) {
      if (i + 1 < arg_tokens.size() && arg_tokens[i].type == TOKEN_TYPE_DOT && i + 1 < arg_tokens.size()) {
        token joined_tok;
        joined_tok.type = TOKEN_TYPE_IDENTIFIER;
        joined_tok.text = arg_tokens[i - 1].text + "." + arg_tokens[i + 1].text;
        joined_tok.line_number = arg_tokens[i - 1].line_number;
        joined_tok.column_number = arg_tokens[i - 1].column_number;
        conjoined_tokens.push_back(joined_tok);
        ++i;
      } else if (arg_tokens[i].type == TOKEN_TYPE_DOT && i + 1 >= arg_tokens.size()) {
        throw std::runtime_error("Unexpected '.' token at end of argument list");
      } else {
        conjoined_tokens.push_back(arg_tokens[i - 1]);
      }
    }

    return conjoined_tokens;
  }

}  // namespace other