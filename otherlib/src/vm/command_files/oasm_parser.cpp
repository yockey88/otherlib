/**
 * \file vm/command_files/oasm_parser.cpp
 **/
#include "vm/command_files/oasm_parser.hpp"

#include <ranges>
#include <span>

#include "core/enum_formatter.hpp"
#include "core/logger.hpp"

#include "vm/opcode.hpp"
#include "vm/other_device.hpp"

#include "code_block.hpp"
#include "token.hpp"

namespace other {
  namespace detail {

    template <typename R>
      requires std::ranges::input_range<R> && std::is_same_v<std::ranges::range_value_t<R>, token>
    std::vector<token> recombine_parameter_tokens(R token_view) {
      std::vector<token> combined_tokens;
      for (auto it = std::ranges::begin(token_view); it != std::ranges::end(token_view);) {
        if ((it->type != TOKEN_TYPE_IDENTIFIER && it->type != TOKEN_TYPE_KW_DATA)) {
          combined_tokens.push_back(*it);
          ++it;
          continue;
        }

        std::string combined_text = it->text;
        auto next_it = std::next(it);
        for (; next_it != std::ranges::end(token_view);) {
          if (next_it->type == TOKEN_TYPE_DOT) {
            combined_text += '.';
            ++next_it;
            if (next_it != std::ranges::end(token_view) && (next_it->type == TOKEN_TYPE_IDENTIFIER || next_it->type == TOKEN_TYPE_KW_DATA)) {
              combined_text += next_it->text;
              ++next_it;
            } else {
              break;
            }
          } else {
            break;
          }
        }

        combined_tokens.push_back(token{ TOKEN_TYPE_IDENTIFIER, combined_text, it->line_number, it->column_number });
        it = next_it;
      }
      return combined_tokens;
    }

    template <typename R>
      requires std::ranges::input_range<R> && std::is_same_v<std::ranges::range_value_t<R>, token>
    std::vector<token> collect_instruction_parameter_tokens(R token_view) {
      std::vector<token> combined_tokens;
      std::vector<token> current_parameter_tokens;

      auto flush_current_parameter = [&combined_tokens, &current_parameter_tokens]() {
        if (current_parameter_tokens.empty()) {
          return;
        }

        auto recombined_parameter = recombine_parameter_tokens(current_parameter_tokens);
        combined_tokens.insert(combined_tokens.end(), recombined_parameter.begin(), recombined_parameter.end());
        current_parameter_tokens.clear();
      };

      for (const auto& tok : token_view) {
        if (tok.type == TOKEN_TYPE_COMMA) {
          flush_current_parameter();
          continue;
        }

        current_parameter_tokens.push_back(tok);
      }

      flush_current_parameter();
      return combined_tokens;
    }

    inline bool is_type_keyword(const token& tok) {
      return (tok.type >= TOKEN_TYPE_KW_I8_TYPE && tok.type <= TOKEN_TYPE_KW_USER_DEFINED_TYPE) ||
        tok.type == TOKEN_TYPE_KW_DATA || tok.type == TOKEN_TYPE_KW_ADDRESS_TYPE;
    }

    inline bool is_instruction_keyword(const token& tok) {
      return tok.type >= TOKEN_TYPE_KW_STOPDEV && tok.type <= TOKEN_TYPE_KW_STOPSCN;
    }

    inline bool is_other_keyword(const token& tok) {
      return tok.type >= TOKEN_TYPE_KW_BEGIN && tok.type <= TOKEN_TYPE_KW_DATA;
    }

    inline bool is_keyword(const token& tok) {
      return std::ranges::find(kKeywordTokens, tok.type, &keyword_token::type) != kKeywordTokens.end();
    }

    inline bool is_eol_marker(const token& tok) {
      return is_instruction_keyword(tok) || tok.type == TOKEN_TYPE_KW_END || tok.type == TOKEN_TYPE_AT;
    }

    static inline auto get_data_object_value_filter() {
      return std::views::take_while([](const token& tok) { return tok.type != TOKEN_TYPE_DOT && tok.type != TOKEN_TYPE_RIGHT_BRACE; }) |
        std::views::transform([](const token& tok) {
               if (tok.type == TOKEN_TYPE_IDENTIFIER) {
                 if (std::ranges::all_of(tok.text, [](char c) { return std::isxdigit(static_cast<unsigned char>(c)); })) {
                   token hex_tok{ TOKEN_TYPE_HEX_LITERAL, tok.text, tok.line_number, tok.column_number };
                   return hex_tok;
                 }
               }
               return tok;
             });
    }

    static inline auto filter_empty_strings() {
      return std::views::filter([](const token& tok) { return !(tok.type == TOKEN_TYPE_STRING_LITERAL && tok.text.empty()); });
    }

  }  // namespace detail

  ocmd_ir oasm_parser::parse() {
    if (tokens.empty()) {
      CORE_LOG_ERROR("No tokens to parse");
      return {};
    }

    if (tokens[0].type != TOKEN_TYPE_SOURCE_START) {
      CORE_LOG_ERROR("First token must be SOURCE_START, but found type {} with text '{}'", static_cast<int>(tokens[0].type), tokens[0].text);
      return {};
    }
    if (tokens.empty()) {
      CORE_LOG_ERROR("No tokens to parse after removing SOURCE_START token");
      return {};
    }
    if (tokens.back().type != TOKEN_TYPE_EOF) {
      CORE_LOG_ERROR("Last token must be EOF, but found type {} with text '{}'", static_cast<int>(tokens.back().type), tokens.back().text);
      return {};
    }

    consume();  // consume SOURCE_START token
    try {
      auto sections = parse_sections();
      process_code_sections(sections.sections);
      process_data_sections(sections.data_sections);
    } catch (const data_object_error& e) {
      CORE_LOG_ERROR("Data object error: {}", e.what());
      return {};
    } catch (const ocmd_parse_error& e) {
      CORE_LOG_ERROR("Parse error: {}", e.what());
      return {};
    } catch (const std::runtime_error& e) {
      CORE_LOG_ERROR("Runtime error during parsing: {}", e.what());
      return {};
    } catch (const std::exception& e) {
      CORE_LOG_ERROR("Unexpected error during parsing at token '{}': {}", current().text, e.what());
      return {};
    } catch (...) {
      CORE_LOG_ERROR("Unknown error during parsing at token '{}'", current().text);
      return {};
    }

    if (!finished()) {
      CORE_LOG_ERROR("Parsing finished but there are still unprocessed tokens starting with type {} and text '{}'", static_cast<int>(current().type), current().text);
      return {};
    }
    OTHER_ASSERT(cursor == tokens.size() - 1, "Expected to be at the last token after parsing, but cursor is at position {} out of {}", cursor, tokens.size());
    OTHER_ASSERT(tokens[cursor].type == TOKEN_TYPE_EOF, "Expected EOF token at end of parsing, but found type {} with text '{}'", static_cast<int>(tokens[cursor].type), tokens[cursor].text);

    ir_result.valid = true;
    return ir_result;
  }

  oasm_parser::section_ir oasm_parser::parse_sections() {
    section_ir sections;

    while (!finished()) {
      if (check(TOKEN_TYPE_DOLLAR)) {
        sections.sections.emplace_back(parse_code_block());
      } else if (check(TOKEN_TYPE_HASH)) {
        consume();
        parse_directive(sections);
      } else if (check(TOKEN_TYPE_SLASH)) {
        consume();
        parse_definition(sections);
      } else {
        sections.sections.emplace_back(parse_code_block());
      }
    }

    return sections;
  }

  void oasm_parser::parse_directive(section_ir& sections) {
    if (!check(TOKEN_TYPE_IDENTIFIER) && !detail::is_keyword(current())) {
      throw ocmd_parse_error(std::format("Expected identifier or keyword after '#' for directive name: type = {}, text = {}", static_cast<int>(current().type), current().text));
    }

    const auto dir = current();
    consume();

    if (finished()) {
      throw ocmd_parse_error("Expected '{' after directive name");
    }
    if (!check(TOKEN_TYPE_LEFT_BRACE)) {
      throw ocmd_parse_error("Expected '{' after directive name");
    }
    consume();  // consume '{'

    if (detail::is_keyword(dir)) {
      parse_keyword_directive(sections, dir);
    } else {
      parse_identifier_directive(sections, dir);
    }

    if (finished()) {
      throw ocmd_parse_error("Expected '}' at end of directive block");
    }
    if (!check(TOKEN_TYPE_RIGHT_BRACE)) {
      throw ocmd_parse_error("Expected '}' at end of directive block");
    }
    consume();  // consume '}'
  }

  void oasm_parser::parse_keyword_directive(section_ir& sections, const token& directive_token) {
    switch (directive_token.type) {
      case TOKEN_TYPE_KW_DATA: sections.data_sections.emplace_back(parse_data_block(directive_token)); break;
      default:
        throw ocmd_parse_error(std::format("Unknown directive '#{}'", directive_token.text));
    }
  }

  void oasm_parser::parse_identifier_directive(section_ir& sections, const token& identifier_token) {
    sections.data_sections.emplace_back(parse_data_block(identifier_token));
  }

  void oasm_parser::parse_definition(section_ir& sections) {
    if (finished()) {
      throw ocmd_parse_error("Expected definition/setting or comment after '/'");
    }
    if (!check(TOKEN_TYPE_IDENTIFIER)) {
      throw ocmd_parse_error(std::format("Expected identifier for definition name after '/', but found type {} with text '{}'", static_cast<int>(current().type), current().text));
    }

    auto& def = ir_result.definitions.emplace_back(compiler_definition{
      .name = current().text,
    });
    consume();

    if (finished()) {
      throw ocmd_parse_error("Expected ':' after definition name");
    }
    if (!check(TOKEN_TYPE_COLON)) {
      throw ocmd_parse_error("Expected ':' after definition name");
    }
    consume();

    if (finished()) {
      throw ocmd_parse_error("Expected value for definition");
    }
    // clang-format off
    if (check(TOKEN_TYPE_KW_BEGIN) || check(TOKEN_TYPE_KW_END) || check(TOKEN_TYPE_KW_DATA) || check(TOKEN_TYPE_KW_ADDRESS_TYPE) ||
        (current().type >= TOKEN_TYPE_KW_I8_TYPE && current().type <= TOKEN_TYPE_KW_USER_DEFINED_TYPE)) {
      throw ocmd_parse_error(std::format("Unexpected keyword '{}' in definition value", current().text));
    }
    // clang-format on

    def.value = current();
    consume();
  }

  oasm_parser::code_section_ir oasm_parser::parse_code_block() {
    code_section_ir section;
    if (check(TOKEN_TYPE_DOLLAR)) {
      // must be $ <name> :
      consume();  // consume the '$'

      if (!check(TOKEN_TYPE_IDENTIFIER)) {
        throw ocmd_parse_error("Expected identifier after '$' for code block name");
      }
      section.name = current().text;
      consume();

      if (!check(TOKEN_TYPE_COLON)) {
        throw ocmd_parse_error("Expected ':' after code block name");
      }
      consume();
    }

    uint32_t instruction_index = 0;

    /// now read everything until END keyword
    while (!finished() && !check(TOKEN_TYPE_KW_END)) {
      bool set_instr_index = false;
      if (check(TOKEN_TYPE_DOLLAR) || check(TOKEN_TYPE_HASH)) {
        // if we just process a return then we are done with the code block and the user simply forgot 'end'
        if (section.instructions.size() > 0 && section.instructions.back().category_and_type == opcode_return()) {
          break;
        }
      }

      if (check(TOKEN_TYPE_AT)) {
        consume();  // consume '@'

        if (finished() || !check(TOKEN_TYPE_IDENTIFIER)) {
          throw ocmd_parse_error("Expected identifier after '@' for label definition");
        }

        section.jump_labels.emplace_back(code_section_ir::jump_label_ir{ .name = current().text });
        set_instr_index = true;
        consume();

        if (finished() || !check(TOKEN_TYPE_COLON)) {
          throw ocmd_parse_error("Expected ':' after label name");
        }
        consume();  // consume ':'
      }

      if (detail::is_instruction_keyword(current())) {
        token curr_token = current();
        uint32_t category_and_type = get_opcode_category_and_type_from_token(curr_token);
        consume();

        auto& instr = section.instructions.emplace_back(code_section_ir::instruction_ir{
          .instruction_index = instruction_index++,
          .category_and_type = category_and_type,
        });
        if (set_instr_index) {
          section.jump_labels.back().instruction_index = instr.instruction_index;
        }

        uint32_t instr_parity = get_instruction_parity(category_and_type);
        if (instr_parity > 0) {
          auto raw_param_tokens = look_from_now() |
            std::views::take_while([](const token& tok) { return !detail::is_eol_marker(tok); }) |
            std::ranges::to<std::vector>();
          for (const auto& _ : raw_param_tokens) {
            consume();
          }

          auto param_tokens = detail::collect_instruction_parameter_tokens(raw_param_tokens);
          const auto params_size = std::ranges::size(param_tokens);
          if (params_size != instr_parity) {
            throw ocmd_parse_error(std::format("Expected {} parameters for instruction '{}', but found {}", instr_parity, curr_token.text, params_size));
          }

          size_t arg_idx = 0;
          for (const auto& param_token : param_tokens) {
            instr.arguments[arg_idx++] = param_token;
          }
        }

      } else {
        consume();
      }
    }

    if (check(TOKEN_TYPE_KW_END)) {
      consume();
    }

    return section;
  }

  oasm_parser::data_section_ir oasm_parser::parse_data_block(const token& directive_token) {
    data_section_ir section;
    section.name = directive_token.text;

    while (!finished()) {
      /// dot implies tag name begin
      if (check(TOKEN_TYPE_RIGHT_BRACE)) {
        break;
      }

      if (check(TOKEN_TYPE_DOLLAR)) {
        throw ocmd_parse_error("Forgot to end data block, expected '}' before code block");
      }
      if (check(TOKEN_TYPE_HASH)) {
        throw ocmd_parse_error("Forgot to end data block, expected '}' before directive block");
      }

      if (check(TOKEN_TYPE_DOT)) {
        consume();  // consume the '.'
        token name;
        token type_label;
        // token value_token;

        if (!check(TOKEN_TYPE_IDENTIFIER)) {
          throw ocmd_parse_error(std::format("Expected identifier for data object name found [{}] instead", current().text));
        }
        name = current();
        consume();

        if (finished()) {
          throw ocmd_parse_error("Expected ':' after data object name");
        }
        if (!check(TOKEN_TYPE_COLON)) {
          throw ocmd_parse_error("Expected ':' after data object name");
        }
        consume();

        if (finished()) {
          throw ocmd_parse_error("Expected type label or '=' after ':' in data object definition found end of file");
        }
        if (detail::is_type_keyword(current())) {
          type_label = current();
          consume();
        }

        if (finished()) {
          throw ocmd_parse_error("Expected '=' after data object definition prefix");
        }
        if (!check(TOKEN_TYPE_EQUAL)) {
          throw ocmd_parse_error("Expected '=' after data object type label found '" + current().text + "'");
        }
        consume();

        /// first check all value types about to be conecatenated to decide type then
        //    collect all tokens until the next dot or closing brace
        auto value_tokens = look_from_now() |
          detail::get_data_object_value_filter() |
          std::ranges::to<std::vector>();
        if (value_tokens.empty()) {
          throw ocmd_parse_error("Expected value for data object, but found none");
        }
        if (value_tokens.back().type == TOKEN_TYPE_EOF) {
          throw ocmd_parse_error(std::format("#{} missing closing '}}'", directive_token.text));
        }

        std::string full_value_text;
        for (const auto& _ : value_tokens) {
          full_value_text += _.text;
          consume();
        }

        if (!check(TOKEN_TYPE_DOT) && !check(TOKEN_TYPE_RIGHT_BRACE)) {
          throw ocmd_parse_error(std::format("Expected end of data object value with '.' or '}}', but found type {} with text '{}'", current().type, current().text));
        }

        data_type final_deduced_type = data_type::OCMD_DATA_TYPE_INVALID;
        if (type_label.type != TOKEN_TYPE_INVALID) {
          final_deduced_type = data_object::data_type_from_label(type_label.text);
        } else {
          final_deduced_type = data_object::deduce_data_type_from_tokens(value_tokens);
        }

        if (type_label.type == TOKEN_TYPE_KW_ADDRESS_TYPE && final_deduced_type == OCMD_DATA_TYPE_BLOB) {
          final_deduced_type = OCMD_DATA_TYPE_ADDRESS;
        }

        if (final_deduced_type == OCMD_DATA_TYPE_ADDRESS) {
          if (value_tokens.size() != 1) {
            throw ocmd_parse_error("Expected a single token for address type, but found " + std::to_string(value_tokens.size()));
          }
          if (value_tokens[0].type != TOKEN_TYPE_HEX_LITERAL) {
            throw ocmd_parse_error("Expected a single hexadecimal literal for address type, but found " + std::to_string(value_tokens.size()) + " tokens");
          }
        }

        if (final_deduced_type == OCMD_DATA_TYPE_BLOB) {
          if (!std::ranges::all_of(value_tokens, [](const token& tok) { return tok.type == TOKEN_TYPE_HEX_LITERAL || tok.type == TOKEN_TYPE_INTEGER_LITERAL; })) {
            throw ocmd_parse_error("Expected all tokens for blob type to be hexadecimal or integer literals");
          }
        }

        if (final_deduced_type == OCMD_DATA_TYPE_STRING) {
          if (value_tokens.size() > 1) {
            if (!std::ranges::all_of(value_tokens, [](const token& tok) { return tok.type == TOKEN_TYPE_STRING_LITERAL; })) {
              throw ocmd_parse_error("Expected all tokens to be string literals for string data type, but found a token of type " + std::to_string(std::ranges::find_if(value_tokens, [](const token& tok) { return tok.type != TOKEN_TYPE_STRING_LITERAL; })->type));
            }
            value_tokens = value_tokens | detail::filter_empty_strings() | std::ranges::to<std::vector>();
          } else {
            if (value_tokens[0].type != TOKEN_TYPE_STRING_LITERAL) {
              throw ocmd_parse_error("Expected string literal for string data type, but found token of type " + std::to_string(value_tokens[0].type));
            }
          }
        }

        token value_token{ TOKEN_TYPE_DATA_VALUE, full_value_text, 0, 0 };
        section.objects.push_back(data_section_ir::data_object_ir{
          .name = name.text,
          .type_label = type_label.text,
          .value_token = value_token,
          .deduced_type = final_deduced_type,
        });
      } else {
        consume();
      }
    }

    if (finished()) {
      throw ocmd_parse_error("Expected '}' at end of data block");
    }

    return section;
  }

  void oasm_parser::process_code_sections(std::vector<code_section_ir>& sections) {
    /// \todo type-checking
    for (auto& section : sections) {
      for (auto& instr : section.instructions) {
        for (auto& arg_token : instr.arguments) {
          if (arg_token.type == TOKEN_TYPE_INVALID) {
            break;
          }

          switch (arg_token.type) {
            case TOKEN_TYPE_INTEGER_LITERAL: break;
            case TOKEN_TYPE_FLOATING_POINT_LITERAL: break;
            case TOKEN_TYPE_STRING_LITERAL: break;
            case TOKEN_TYPE_IDENTIFIER: arg_token.type = TOKEN_TYPE_LABEL; break;
            case TOKEN_TYPE_HEX_LITERAL:
              arg_token.text = arg_token.text.substr(2);
              arg_token.type = TOKEN_TYPE_ADDRESS;
              break;
            default:
              if (arg_token.type >= TOKEN_TYPE_KW_R0 && arg_token.type <= TOKEN_TYPE_KW_RFLAG) {
              } else {
                CORE_LOG_ERROR("Unexpected token type [{}] for argument '{}'", static_cast<int>(arg_token.type), arg_token.text);
              }
              break;
          }
        }
      }
    }

    /// now go through and finalize instructions into code block for final IR
    for (auto& section : sections) {
      auto& code_blk = ir_result.code_blocks.emplace_back(code_block{});
      code_blk.name = section.name;
      if (code_blk.name.empty()) {
        code_blk.name = std::format("code_block_{}", ir_result.code_blocks.size() - 1);
      }

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

  void oasm_parser::process_data_sections(std::vector<data_section_ir>& sections) {
    for (auto& data_section : sections) {
      auto itr = std::ranges::find(ir_result.data_blocks, data_section.name, &data_block::name);
      if (itr == ir_result.data_blocks.end()) {
        ir_result.data_blocks.emplace_back(data_block{});
        itr = std::prev(ir_result.data_blocks.end());
        itr->name = data_section.name;
      }
      auto& data_blk = *itr;

      for (auto& obj_ir : data_section.objects) {
        auto itr = std::ranges::find(data_blk.objects, obj_ir.name, &data_object::name);
        if (itr != data_blk.objects.end()) {
          throw ocmd_parse_error(std::format("Duplicate data object name '{}' in data block '{}'", obj_ir.name, data_blk.name));
        }

        auto& obj = data_blk.objects.emplace_back();
        obj.name = obj_ir.name;
        obj.value_token = obj_ir.value_token;
        obj.data = data_object::data_from_token_and_type(obj.value_token, obj_ir.deduced_type);

        if (!obj_ir.type_label.empty()) {
          obj.type = data_object::data_type_from_label(obj_ir.type_label);
        } else {
          obj.type = obj_ir.deduced_type;
        }
      }
    }
  }

  const token& oasm_parser::peek(size_t offset) const {
    if (finished()) {
      static token eof_token{ TOKEN_TYPE_EOF, "", static_cast<size_t>(-1), static_cast<size_t>(-1) };
      return eof_token;
    } else {
      return tokens[cursor + offset];
    }
  }

  const token& oasm_parser::current() const {
    return peek(0);
  }

  const std::span<const token> oasm_parser::look_from_now(size_t count) const {
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

  bool oasm_parser::finished() const {
    return cursor >= tokens.size() || tokens[cursor].type == TOKEN_TYPE_EOF;
  }

  void oasm_parser::consume() {
    cursor++;
  }

  bool oasm_parser::check(token_type type) const {
    return current().type == type;
  }

  bool oasm_parser::check_next(token_type type) const {
    return peek(1).type == type;
  }

  uint32_t oasm_parser::get_instruction_parity(uint32_t category_and_type) const {
    /// for now we can just hardcode this based on the opcode definitions, but ideally this should be determined by the instruction metadata
    uint8_t category = get_category_nibble(category_and_type);
    uint8_t type = get_type_nibble(category_and_type);

    switch (category) {
      // all device control instructions take 1 argument
      case OPCODE_CATEGORY_DEVICE_CONTROL: return 1;
      case OPCODE_CATEGORY_LOAD_STORE_LOGICAL:
        if (type <= 0x03) {
          return 2;  // write, load, set, iwrite take 2 arguments
        } else if (type <= 0x0B) {
          return 3;  // cmp, cmpgt, cmplt, and, or, xor, lshift, rshift take 3 arguments
        }
        break;
      case OPCODE_CATEGORY_PROGRAM_FLOW:
        // ret takes 0 arguments
        if (type == 0x04) {
          return 0;
        } else {
          return 1;
        }
        break;
      case OPCODE_CATEGORY_ARITHMETIC:
        if (type <= 0x04) {
          return 3;  // add, sub, mul, div, mod take 3 arguments
        }
        break;
      // all scene table instructions take 1 argument
      case OPCODE_CATEGORY_SCENE_TABLE: return 1;
      default:
        CORE_LOG_ERROR("Unknown opcode category [{}] for parity calculation", category);
        break;
    }

    CORE_LOG_ERROR("Unable to determine instruction parity for category [{}] and type [{}]", category, type);
    return 0;  // default to parity of 0 if unknown
  }

  uint32_t oasm_parser::get_opcode_category_and_type_from_token(const token& tok) const {
    if (tok.type < TOKEN_TYPE_KW_STOPDEV || tok.type > TOKEN_TYPE_KW_LOADSCN) {
      throw ocmd_parse_error(std::format("Token '{}' is not a valid instruction keyword", tok.text));
    }

    switch (tok.type) {
      case TOKEN_TYPE_KW_STOPDEV: return opcode_with_category_and_type(OPCODE_CATEGORY_DEVICE_CONTROL, 0x00);
      case TOKEN_TYPE_KW_DUMP: return opcode_with_category_and_type(OPCODE_CATEGORY_DEVICE_CONTROL, 0x01);
      case TOKEN_TYPE_KW_DUMPX: return opcode_with_category_and_type(OPCODE_CATEGORY_DEVICE_CONTROL, 0x02);
      case TOKEN_TYPE_KW_DUMPMEM: return opcode_with_category_and_type(OPCODE_CATEGORY_DEVICE_CONTROL, 0x03);
      case TOKEN_TYPE_KW_WRITE: return opcode_with_category_and_type(OPCODE_CATEGORY_LOAD_STORE_LOGICAL, 0x00);
      case TOKEN_TYPE_KW_LOAD: return opcode_with_category_and_type(OPCODE_CATEGORY_LOAD_STORE_LOGICAL, 0x01);
      case TOKEN_TYPE_KW_SET: return opcode_with_category_and_type(OPCODE_CATEGORY_LOAD_STORE_LOGICAL, 0x02);
      case TOKEN_TYPE_KW_IWRITE: return opcode_with_category_and_type(OPCODE_CATEGORY_LOAD_STORE_LOGICAL, 0x03);
      case TOKEN_TYPE_KW_CMP: return opcode_with_category_and_type(OPCODE_CATEGORY_LOAD_STORE_LOGICAL, 0x04);
      case TOKEN_TYPE_KW_CMPGT: return opcode_with_category_and_type(OPCODE_CATEGORY_LOAD_STORE_LOGICAL, 0x05);
      case TOKEN_TYPE_KW_CMPLT: return opcode_with_category_and_type(OPCODE_CATEGORY_LOAD_STORE_LOGICAL, 0x06);
      case TOKEN_TYPE_KW_AND: return opcode_with_category_and_type(OPCODE_CATEGORY_LOAD_STORE_LOGICAL, 0x07);
      case TOKEN_TYPE_KW_OR: return opcode_with_category_and_type(OPCODE_CATEGORY_LOAD_STORE_LOGICAL, 0x08);
      case TOKEN_TYPE_KW_XOR: return opcode_with_category_and_type(OPCODE_CATEGORY_LOAD_STORE_LOGICAL, 0x09);
      case TOKEN_TYPE_KW_LSHIFT: return opcode_with_category_and_type(OPCODE_CATEGORY_LOAD_STORE_LOGICAL, 0x0A);
      case TOKEN_TYPE_KW_RSHIFT: return opcode_with_category_and_type(OPCODE_CATEGORY_LOAD_STORE_LOGICAL, 0x0B);
      case TOKEN_TYPE_KW_GOTO: return opcode_with_category_and_type(OPCODE_CATEGORY_PROGRAM_FLOW, 0x00);
      case TOKEN_TYPE_KW_JE: return opcode_with_category_and_type(OPCODE_CATEGORY_PROGRAM_FLOW, 0x01);
      case TOKEN_TYPE_KW_JNE: return opcode_with_category_and_type(OPCODE_CATEGORY_PROGRAM_FLOW, 0x02);
      case TOKEN_TYPE_KW_CALL: return opcode_with_category_and_type(OPCODE_CATEGORY_PROGRAM_FLOW, 0x03);
      case TOKEN_TYPE_KW_RET: return opcode_with_category_and_type(OPCODE_CATEGORY_PROGRAM_FLOW, 0x04);
      case TOKEN_TYPE_KW_RETX: return opcode_with_category_and_type(OPCODE_CATEGORY_PROGRAM_FLOW, 0x05);
      case TOKEN_TYPE_KW_ADD: return opcode_with_category_and_type(OPCODE_CATEGORY_ARITHMETIC, 0x00);
      case TOKEN_TYPE_KW_SUB: return opcode_with_category_and_type(OPCODE_CATEGORY_ARITHMETIC, 0x01);
      case TOKEN_TYPE_KW_MUL: return opcode_with_category_and_type(OPCODE_CATEGORY_ARITHMETIC, 0x02);
      case TOKEN_TYPE_KW_DIV: return opcode_with_category_and_type(OPCODE_CATEGORY_ARITHMETIC, 0x03);
      case TOKEN_TYPE_KW_MOD: return opcode_with_category_and_type(OPCODE_CATEGORY_ARITHMETIC, 0x04);
      case TOKEN_TYPE_KW_LOADSCN: return opcode_with_category_and_type(OPCODE_CATEGORY_SCENE_TABLE, 0x00);
      case TOKEN_TYPE_KW_PLAYSCN: return opcode_with_category_and_type(OPCODE_CATEGORY_SCENE_TABLE, 0x01);
      case TOKEN_TYPE_KW_STOPSCN: return opcode_with_category_and_type(OPCODE_CATEGORY_SCENE_TABLE, 0x02);
      default:
        throw ocmd_parse_error(std::format("Unhandled instruction keyword token type [{}]", static_cast<int>(tok.type)));
    }
  }

}  // namespace other