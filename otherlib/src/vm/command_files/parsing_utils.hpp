/**
 * \file vm/command_files/parsing_utils.hpp
 **/
#ifndef OTHERLIB_VM_COMMAND_FILES_PARSING_UTILS_HPP
#define OTHERLIB_VM_COMMAND_FILES_PARSING_UTILS_HPP

#include <ranges>

#include "vm/command_files/data_block.hpp"
#include "vm/command_files/token.hpp"
#include "vm/operand.hpp"

namespace other {

  struct code_section_ir {
    struct instruction_ir {
      constexpr static size_t kMaxArguments = 3;
      uint32_t instruction_index = 0;
      canonical_opcode opcode = canonical_opcode::INVALID_OP;
      token arguments[kMaxArguments] = {
        token{ TOKEN_TYPE_INVALID, "", source_span{ { 0, 0 }, { 0, 0 } } },
        token{ TOKEN_TYPE_INVALID, "", source_span{ { 0, 0 }, { 0, 0 } } },
        token{ TOKEN_TYPE_INVALID, "", source_span{ { 0, 0 }, { 0, 0 } } }
      };
    };
    struct jump_label_ir {
      std::string name;
      uint32_t instruction_index = 0;
      uint16_t section_address = 0;
    };

    std::string name;
    ostd::vector<instruction_ir> instructions = {};
    ostd::vector<jump_label_ir> jump_labels = {};
  };

  struct data_section_ir {
    struct data_object_ir {
      std::string name;
      std::string type_label;
      token value_token;
      data_type deduced_type = OCMD_DATA_TYPE_INVALID;
    };

    std::string name;
    ostd::vector<data_object_ir> objects = {};
  };

  struct section_ir {
    ostd::vector<code_section_ir> sections = {};
    ostd::vector<data_section_ir> data_sections = {};
  };

  namespace detail {

    template <typename R>
      requires std::ranges::input_range<R> && std::is_same_v<std::ranges::range_value_t<R>, token>
    ostd::vector<token> recombine_parameter_tokens(R token_view) {
      ostd::vector<token> combined_tokens;
      for (auto it = std::ranges::begin(token_view); it != std::ranges::end(token_view);) {
        if (it->type == TOKEN_TYPE_LEFT_BRACKET) {
          token accessed_token = *it;
          it = std::ranges::next(it);
          // combine everything until ']'
          token_type first_tok_type = it != std::ranges::end(token_view) ? it->type : TOKEN_TYPE_EOF;
          if (first_tok_type == TOKEN_TYPE_EOF) {
            CORE_LOG_ERROR("Unexpected end of tokens while recombining parameter tokens for instruction parameter starting with '{}'", accessed_token.text);
            break;
          }

          std::string combined_text = accessed_token.text;
          for (; it != std::ranges::end(token_view); ++it) {
            combined_text += it->text;
            if (it->type == TOKEN_TYPE_RIGHT_BRACKET) {
              break;
            }
          }
          if (it == std::ranges::end(token_view)) {
            CORE_LOG_ERROR("Unexpected end of tokens while recombining parameter tokens for instruction parameter starting with '{}'", accessed_token.text);
            break;
          }

          token_type final_type = first_tok_type;
          if (first_tok_type == TOKEN_TYPE_KW_DATA) {
            final_type = TOKEN_TYPE_IDENTIFIER;
          }
          combined_tokens.push_back(token{ final_type, combined_text, accessed_token.source_view });
          if (it != std::ranges::end(token_view)) {
            ++it;
          }
          continue;
        } else if (it->type == TOKEN_TYPE_STRING_LITERAL) {
          CORE_LOG_ERROR("STRINGS UNIMPLEMENTED IN RECOMBINE, THIS SHOULD NOT HAPPEN");
          combined_tokens.push_back(*it);
          ++it;
          continue;
        } else if ((it->type != TOKEN_TYPE_IDENTIFIER && it->type != TOKEN_TYPE_KW_DATA)) {
          combined_tokens.push_back(*it);
          ++it;
          continue;
        }

        std::string combined_text = it->text;
        auto next_it = std::next(it);
        if (next_it == std::ranges::end(token_view)) {
          combined_tokens.push_back(*it);
          break;
        }

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

        combined_tokens.push_back(token{ TOKEN_TYPE_IDENTIFIER, combined_text, it->source_view });
        it = next_it;
      }
      return combined_tokens;
    }

    template <typename R>
      requires std::ranges::input_range<R> && std::is_same_v<std::ranges::range_value_t<R>, token>
    ostd::vector<token> collect_instruction_parameter_tokens(R token_view) {
      ostd::vector<token> combined_tokens;
      ostd::vector<token> current_parameter_tokens;

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
      return tok.type >= TOKEN_TYPE_KW_STOPDEV && tok.type <= TOKEN_TYPE_KW_MOD;
    }

    inline bool is_other_keyword(const token& tok) {
      return tok.type >= TOKEN_TYPE_KW_BEGIN && tok.type <= TOKEN_TYPE_KW_DATA;
    }

    inline bool is_keyword(const token& tok) {
      return std::ranges::find(kKeywordTokens, tok.type, &keyword_token::type) != kKeywordTokens.end();
    }

    inline bool is_next_section_marker(const token& tok) {
      return tok.type == TOKEN_TYPE_HASH || tok.type == TOKEN_TYPE_DOLLAR || tok.type == TOKEN_TYPE_SLASH;
    }

    inline bool end_of_code_section(const token& tok) {
      return tok.type == TOKEN_TYPE_KW_END || tok.type == TOKEN_TYPE_EOF;
    }

    inline bool is_eol_marker(const token& tok) {
      return is_instruction_keyword(tok) ||  // next instruction
        is_next_section_marker(tok) ||       // next section but missing 'end' for code block
        tok.type == TOKEN_TYPE_KW_END ||     // end of code block
        tok.type == TOKEN_TYPE_AT ||         // for tags/labels and missing 'end'
        tok.type == TOKEN_TYPE_EOF;
    }

    static inline auto get_data_object_value_filter() {
      return std::views::take_while([](const token& tok) { return tok.type != TOKEN_TYPE_DOT && tok.type != TOKEN_TYPE_RIGHT_BRACE; }) |
        std::views::transform([](const token& tok) {
               if (tok.type == TOKEN_TYPE_IDENTIFIER) {
                 if (std::ranges::all_of(tok.text, [](char c) { return std::isxdigit(static_cast<unsigned char>(c)); })) {
                   token hex_tok{ TOKEN_TYPE_HEX_LITERAL, tok.text, tok.source_view };
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
}  // namespace other

#endif  // OTHERLIB_VM_COMMAND_FILES_PARSING_UTILS_HPP