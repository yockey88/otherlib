/**
 * \file vm/command_files/parser.cpp
 **/
#include "vm/command_files/parser.hpp"

#include <ranges>
#include <span>

#include "core/logger.hpp"

#include "vm/opcode.hpp"
#include "vm/other_device.hpp"

#include "code_block.hpp"
#include "token.hpp"

namespace other {

  struct parse_error : public std::runtime_error {
    parse_error(const std::string& msg)
        : std::runtime_error(msg) {}
  };

  ocmd_ir ocmd_parser::parse() {
    try {
      auto sections = parse_sections();
      process_code_sections(sections.sections);
      process_data_sections(sections.data_sections);
    } catch (const parse_error& e) {
      CORE_LOG_ERROR("Parse error: {}", e.what());
      return {};
    } catch (const std::exception& e) {
      CORE_LOG_ERROR("Unexpected error during parsing at token '{}': {}", current().text, e.what());
      return {};
    } catch (...) {
      CORE_LOG_ERROR("Unknown error during parsing at token '{}'", current().text);
      return {};
    }

    ir_result.valid = true;
    return ir_result;
  }

  ocmd_parser::block_section_ir ocmd_parser::parse_sections() {
    block_section_ir sections;

    while (!finished()) {
      /// call-label definition
      if (check(TOKEN_TYPE_DOLLAR)) {
        sections.sections.emplace_back(parse_code_block());
      }
      /// data-block definition
      else if (check(TOKEN_TYPE_HASH)) {
        sections.data_sections.emplace_back(parse_data_block());
      } else if (check(TOKEN_TYPE_SLASH)) {
        consume();  // consume '/'
        if (finished() || !check(TOKEN_TYPE_IDENTIFIER)) {
          throw parse_error("Expected definition/setting or comment after '/'");
        }

        auto& def = ir_result.definitions.emplace_back(ocmd_ir::definition{
          .name = current().text,
        });
        consume();

        if (finished() || !check(TOKEN_TYPE_COLON)) {
          throw parse_error("Expected':' after definition name");
        }
        consume();  // consume ':'

        if (finished()) {
          throw parse_error("Expected value for definition");
        }

        def.value = current();
        consume();
      } else {
        throw parse_error("Unexpected token: " + current().text);
      }
    }

    return sections;
  }

  ocmd_parser::code_section_ir ocmd_parser::parse_code_block() {
    if (!check(TOKEN_TYPE_DOLLAR)) {
      throw std::runtime_error("Expected '$' at start of code block");
    }
    consume();  // consume the '$'

    code_section_ir section;

    if (finished() || !check(TOKEN_TYPE_IDENTIFIER)) {
      throw parse_error("Expected identifier after '$' for code block name");
    }

    section.name = current().text;
    consume();
    if (finished() || !check(TOKEN_TYPE_COLON)) {
      throw parse_error("Expected ':' after code block name");
    }
    consume();

    uint32_t instruction_index = 0;

    /// now read everything until END keyword
    while (!finished() && !check(TOKEN_TYPE_KW_END)) {
      bool set_instr_index = false;
      if (check(TOKEN_TYPE_AT)) {
        consume();  // consume '@'

        if (finished() || !check(TOKEN_TYPE_IDENTIFIER)) {
          throw parse_error("Expected identifier after '@' for label definition");
        }

        section.jump_labels.emplace_back(code_section_ir::jump_label_ir{ .name = current().text });
        set_instr_index = true;
        consume();

        if (finished() || !check(TOKEN_TYPE_COLON)) {
          throw parse_error("Expected ':' after label name");
        }
        consume();  // consume ':'
      }

      if (is_instruction_keyword(current())) {
        auto curr_token = current();
        uint32_t category_and_type = get_opcode_category_and_type_from_token(curr_token);
        consume();

        /// TODO:
        // if (curr_token.type == TOKEN_TYPE_KW_END) {
        //   /// if the last instruction emitted was RET or RETX, we can safely ignore the END,
        //   ///   if not then we silently add a RET instruction at the end
        //   if (section.instructions.empty() || !is_return_instruction(section.instructions.back())) {
        //     CORE_LOG_WARNING("Code block '{}' missing explicit 'ret' before 'end', adding implicit 'ret'", section.name);
        //     auto& instr = section.instructions.emplace_back(code_section_ir::instruction_ir{
        //       .instruction_index = instruction_index++,
        //       .category_and_type = raw_instruction::category_and_type_from_opcode(other_command_device::OPCODE_RET),
        //     });
        //   }
        //   break;
        // }

        auto rem_tokens = look_from_now() |
          std::views::take_while([this](const token& tok) { return !is_eol_marker(tok); }) |
          std::ranges::to<std::vector>();
        auto instr_tokens = rem_tokens |
          std::views::filter([this](const token& tok) { return tok.type != TOKEN_TYPE_COMMA; }) |
          std::ranges::to<std::vector>();

        auto& instr = section.instructions.emplace_back(code_section_ir::instruction_ir{
          .instruction_index = instruction_index++,
          .category_and_type = category_and_type,
        });
        if (set_instr_index) {
          section.jump_labels.back().instruction_index = instr.instruction_index;
        }

        // std::vector<token> arg_tokens = raw_instruction::get_argument_tokens_for_instruction(category_and_type, instr_tokens);
        // for (natural_t i = 0; i < arg_tokens.size(); ++i) {
        //   instr.arguments[i] = arg_tokens[i];
        // }
      } else {
        consume();
      }
    }

    if (finished() || !check(TOKEN_TYPE_KW_END)) {
      throw parse_error("Expected 'end' keyword at end of code block");
    }
    consume();

    return section;
  }

  ocmd_parser::data_section_ir ocmd_parser::parse_data_block() {
    if (!check(TOKEN_TYPE_HASH)) {
      throw parse_error("Expected '#' at start of data block");
    }
    consume();  // consume the '#'

    data_section_ir section;

    if (finished() || !check(TOKEN_TYPE_IDENTIFIER)) {
      throw parse_error("Expected identifier after '#' for data block name");
    }

    section.name = current().text;
    consume();

    if (finished() || !check(TOKEN_TYPE_LEFT_BRACE)) {
      throw parse_error("Expected '{' after data block name");
    }
    consume();

    while (!finished()) {
      /// dot implies tag name begin
      if (check(TOKEN_TYPE_DOT)) {
        consume();
        std::string name;
        std::string type_label;
        // token value_token;

        if (!check(TOKEN_TYPE_IDENTIFIER)) {
          throw parse_error(std::format("Expected identifier for data object name found [{}] instead", current().text));
        }
        name = current().text;
        consume();

        if (finished() || !check(TOKEN_TYPE_COLON)) {
          throw parse_error("Expected ':' after data object name");
        }
        consume();

        if (finished()) {
          throw parse_error("Expected type label for data object");
        }

        if (!check(TOKEN_TYPE_EQUAL) && is_type_keyword(current())) {
          type_label = current().text;
          consume();
        }

        if (finished() || !check(TOKEN_TYPE_EQUAL)) {
          throw parse_error("Expected '=' after data object type label");
        }
        consume();

        /// first check all value types about to be conecatenated to decide type then
        //    collect all tokens until the next dot or closing brace
        auto value_tokens = look_from_now() |
          std::views::take_while([this](const token& tok) { return tok.type != TOKEN_TYPE_DOT && tok.type != TOKEN_TYPE_RIGHT_BRACE; }) |
          std::views::transform([this](const token& tok) {
                              if (tok.type == TOKEN_TYPE_IDENTIFIER) {
                                if (std::ranges::all_of(tok.text, [](char c) { return std::isxdigit(static_cast<unsigned char>(c)); })) {
                                  token hex_tok{ TOKEN_TYPE_HEX_LITERAL, tok.text, tok.line_number, tok.column_number };
                                  return hex_tok;
                                }
                              }
                              return tok;
                            }) |
          std::ranges::to<std::vector>();
        data_type type = data_object::deduce_data_type_from_tokens(value_tokens);

        std::string concat_value;
        while (!finished() && !check(TOKEN_TYPE_DOT) && !check(TOKEN_TYPE_RIGHT_BRACE)) {
          concat_value += current().text;
          consume();
          if (!check(TOKEN_TYPE_DOT) && !check(TOKEN_TYPE_RIGHT_BRACE)) {
            concat_value += " ";
          }
        }

        token value_token{ TOKEN_TYPE_DATA_VALUE, concat_value, 0, 0 };
        section.objects.push_back(data_section_ir::data_object_ir{
          .name = name,
          .type_label = type_label,
          .value_token = value_token,
          .deduced_type = type,
        });
      } else if (check(TOKEN_TYPE_RIGHT_BRACE)) {
        break;
      } else {
        consume();
      }
    }

    if (finished() || !check(TOKEN_TYPE_RIGHT_BRACE)) {
      throw parse_error("Expected '}' at end of data block");
    }
    consume();

    return section;
  }

  void ocmd_parser::process_code_sections(std::vector<code_section_ir>& sections) {
    /// \todo type-checking
    for (auto& section : sections) {
      for (auto& instr : section.instructions) {
        for (auto& arg_token : instr.arguments) {
          if (arg_token.type == TOKEN_TYPE_INVALID) {
            break;
          }

          switch (arg_token.type) {
            case TOKEN_TYPE_REGISTER: break;
            case TOKEN_TYPE_INTEGER_LITERAL: break;
            case TOKEN_TYPE_FLOATING_POINT_LITERAL: break;
            case TOKEN_TYPE_STRING_LITERAL: break;
            case TOKEN_TYPE_IDENTIFIER: arg_token.type = TOKEN_TYPE_LABEL; break;
            case TOKEN_TYPE_HEX_LITERAL:
              arg_token.text = arg_token.text.substr(2);
              arg_token.type = TOKEN_TYPE_ADDRESS;
              break;
            default:
              CORE_LOG_ERROR("Unexpected token type [{}] for argument '{}'", arg_token.type, arg_token.text);
              break;
          }
        }
      }
    }

    /// now go through and finalize instructions into code block for final IR
    for (auto& section : sections) {
      auto& code_blk = ir_result.code_blocks.emplace_back(code_block{});
      code_blk.name = section.name;

      for (auto& instr_ir : section.instructions) {
        auto& instr = code_blk.instructions.emplace_back();
        instr.category_and_type = instr_ir.category_and_type;

        for (const auto& arg_tok : instr_ir.arguments) {
          if (arg_tok.type == TOKEN_TYPE_INVALID) {
            break;
          }

          instr.arguments.push_back(raw_instruction::argument::from_token(arg_tok));
        }
      }

      for (const auto& lbl_ir : section.jump_labels) {
        auto& lbl = code_blk.jump_labels.emplace_back();
        lbl.name = lbl_ir.name;
        lbl.section_address = static_cast<uint16_t>(lbl_ir.instruction_index * other_command_device::kOpCodeSize);
      }
    }
  }

  void ocmd_parser::process_data_sections(std::vector<data_section_ir>& sections) {
    for (auto& data_section : sections) {
      auto& data_blk = ir_result.data_blocks.emplace_back(data_block{});
      data_blk.name = data_section.name;

      for (auto& obj_ir : data_section.objects) {
        auto& obj = data_blk.objects.emplace_back();
        obj.name = obj_ir.name;
        obj.value_token = obj_ir.value_token;

        if (!obj_ir.type_label.empty()) {
          obj.type = data_object::data_type_from_label(obj_ir.type_label);
        } else {
          obj.type = obj_ir.deduced_type;
        }
      }
    }

    /// now get data for each object
    for (auto& data_blk : ir_result.data_blocks) {
      for (auto& obj : data_blk.objects) {
        if (obj.value_token.type != TOKEN_TYPE_DATA_VALUE) {
          CORE_LOG_ERROR("Data object '{}' has invalid value token type [{}]", obj.name, static_cast<int>(obj.value_token.type));
          continue;
        }

        obj.data = data_object::data_from_token_and_type(obj.value_token, obj.type);
      }
    }
  }

  bool ocmd_parser::is_type_keyword(const token& tok) const {
    return tok.type >= TOKEN_TYPE_KW_I8_TYPE && tok.type <= TOKEN_TYPE_KW_USER_DEFINED_TYPE;
  }

  bool ocmd_parser::is_instruction_keyword(const token& tok) const {
    return tok.type >= TOKEN_TYPE_KW_STOPDEV && tok.type <= TOKEN_TYPE_KW_LOADSCN;
  }

  bool ocmd_parser::is_eol_marker(const token& tok) const {
    return is_instruction_keyword(tok) || tok.type == TOKEN_TYPE_KW_END || tok.type == TOKEN_TYPE_AT;
  }

  // bool ocmd_parser::is_return_instruction(uint32_t category_and_type) const {
  //   /// this works because these take
  //   return category_and_type == OPCODE_RETURN || category_and_type == OPCODE_RETURN_VALUE_IN_X;
  // }

  const token& ocmd_parser::peek(size_t offset) const {
    if (finished()) {
      static token eof_token{ TOKEN_TYPE_EOF, "", static_cast<size_t>(-1), static_cast<size_t>(-1) };
      return eof_token;
    } else {
      return tokens[cursor + offset];
    }
  }

  const token& ocmd_parser::current() const {
    return peek(0);
  }

  const std::span<const token> ocmd_parser::look_from_now(size_t count) const {
    if (finished()) {
      return {};
    }

    if (count == 0) {
      return std::span(tokens).subspan(cursor);
    } else {
      size_t available = tokens.size() - cursor;
      size_t to_take = std::min(count, available);
      return std::span(tokens).subspan(cursor, to_take);
    }
  }

  bool ocmd_parser::finished() const {
    return cursor >= tokens.size();
  }

  void ocmd_parser::consume() {
    cursor++;
  }

  bool ocmd_parser::check(token_type type) const {
    return current().type == type;
  }

  bool ocmd_parser::check_next(token_type type) const {
    return peek(1).type == type;
  }

  uint32_t ocmd_parser::get_opcode_category_and_type_from_token(const token& tok) const {
    // clang-format off
    // if (tok.type == TOKEN_TYPE_KW_STOPDEV) { return OPCODE_STOPDEV; }
    // if (tok.type == TOKEN_TYPE_KW_DUMP) { return OPCODE_DUMP; }
    // if (tok.type == TOKEN_TYPE_KW_DUMPX) { return OPCODE_DUMPX; }
    // if (tok.type == TOKEN_TYPE_KW_WRITE) { return OPCODE_WRITE_X_TO_MEM; }
    // if (tok.type == TOKEN_TYPE_KW_LOAD) { return OPCODE_LOAD_X_FROM_MEM; }
    // if (tok.type == TOKEN_TYPE_KW_SET) { return OPCODE_LOAD_X_DIRECT; }
    // if (tok.type == TOKEN_TYPE_KW_IWRITE) { return OPCODE_INDIRECT_WRITE_X_TO_MEM; }
    // if (tok.type == TOKEN_TYPE_KW_CMP) { return OPCODE_COMPARE_X_Y_SET_Z; }
    // if (tok.type == TOKEN_TYPE_KW_CMPGT) { return OPCODE_COMPARE_GT_X_Y_SET_Z; }
    // if (tok.type == TOKEN_TYPE_KW_CMPLT) { return OPCODE_COMPARE_LT_X_Y_SET_Z; }
    // if (tok.type == TOKEN_TYPE_KW_AND) { return OPCODE_X_AND_Y_SET_Z; }
    // if (tok.type == TOKEN_TYPE_KW_OR) { return OPCODE_X_OR_Y_SET_Z; }
    // if (tok.type == TOKEN_TYPE_KW_XOR) { return OPCODE_X_XOR_Y_SET_Z; }
    // if (tok.type == TOKEN_TYPE_KW_LSHIFT) { return OPCODE_SHIFT_LEFT_X_BY_Y; }
    // if (tok.type == TOKEN_TYPE_KW_RSHIFT) { return OPCODE_SHIFT_RIGHT_X_BY_Y; }
    // if (tok.type == TOKEN_TYPE_KW_GOTO || 
    //     tok.type == TOKEN_TYPE_KW_JMP) { return OPCODE_GOTO; }
    // if (tok.type == TOKEN_TYPE_KW_JE) { return OPCODE_JUMP_IF_ZERO; }
    // if (tok.type == TOKEN_TYPE_KW_JNE) { return OPCODE_JUMP_IF_NOT_ZERO; }
    // if (tok.type == TOKEN_TYPE_KW_CALL) { return OPCODE_CALL_AT; }
    // if (tok.type == TOKEN_TYPE_KW_RET) { return OPCODE_RETURN; }
    // if (tok.type == TOKEN_TYPE_KW_RETX) { return OPCODE_RETURN_VALUE_IN_X; }
    // if (tok.type == TOKEN_TYPE_KW_ADD) { return OPCODE_ADD_X_Y_TO_X; }
    // if (tok.type == TOKEN_TYPE_KW_SUB) { return OPCODE_SUB_X_Y_TO_X; }
    // if (tok.type == TOKEN_TYPE_KW_MUL) { return OPCODE_MUL_X_Y_TO_X; }
    // if (tok.type == TOKEN_TYPE_KW_DIV) { return OPCODE_DIV_X_Y_TO_X; }
    // if (tok.type == TOKEN_TYPE_KW_MOD) { return OPCODE_MOD_X_Y_TO_X; }
    // if (tok.type == TOKEN_TYPE_KW_LOADSCN) { return OPCODE_LOAD_SCENE_WITH_ID_AT; }
    // clang-format on
    throw parse_error("Unknown opcode token: " + tok.text);
  }

}  // namespace other