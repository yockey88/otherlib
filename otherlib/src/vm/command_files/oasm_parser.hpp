/**
 * \file vm/command_files/oasm_parser.hpp
 **/
#ifndef OTHERLIB_VM_COMMAND_FILES_OASM_PARSER_HPP
#define OTHERLIB_VM_COMMAND_FILES_OASM_PARSER_HPP

#include <span>
#include <string>
#include <vector>

#include "vm/command_files/ocmd_ir.hpp"
#include "vm/command_files/token.hpp"

namespace other {

  class ocmd_parse_error : public std::runtime_error {
   public:
    ocmd_parse_error(const std::string& msg)
        : std::runtime_error(msg) {}
  };

  class oasm_parser {
   public:
    oasm_parser(const std::vector<token>& tokens)
        : tokens(tokens) {}
    ~oasm_parser() = default;

    ocmd_ir parse();

   private:
    struct code_section_ir {
      struct instruction_ir {
        constexpr static size_t kMaxArguments = 3;
        uint32_t instruction_index = 0;
        uint32_t category_and_type = 0;
        token arguments[kMaxArguments] = {
          token{ TOKEN_TYPE_INVALID, "", 0, 0 },
          token{ TOKEN_TYPE_INVALID, "", 0, 0 },
          token{ TOKEN_TYPE_INVALID, "", 0, 0 }
        };
      };
      struct jump_label_ir {
        std::string name;
        uint32_t instruction_index = 0;
        uint16_t section_address = 0;
      };

      std::string name;
      std::vector<instruction_ir> instructions = {};
      std::vector<jump_label_ir> jump_labels = {};
    };

    struct data_section_ir {
      struct data_object_ir {
        std::string name;
        std::string type_label;
        token value_token;
        data_type deduced_type = OCMD_DATA_TYPE_INVALID;
      };

      std::string name;
      std::vector<data_object_ir> objects = {};
    };

    struct section_ir {
      std::vector<code_section_ir> sections = {};
      std::vector<data_section_ir> data_sections = {};
    };
    ocmd_ir ir_result;

    std::vector<token> tokens;

    size_t cursor = 0;

    section_ir parse_sections();
    void parse_directive(section_ir& sections);
    void parse_keyword_directive(section_ir& sections, const token& directive_token);
    void parse_identifier_directive(section_ir& sections, const token& identifier_token);
    void parse_definition(section_ir& sections);

    code_section_ir parse_code_block();
    data_section_ir parse_data_block(const token& directive_token);

    void process_code_sections(std::vector<code_section_ir>& sections);
    void process_data_sections(std::vector<data_section_ir>& sections);

    const token& peek(size_t offset) const;
    const token& current() const;

    const std::span<const token> look_from_now(size_t count = 0) const;

    void consume();

    bool finished() const;

    bool check(token_type type) const;
    bool check_next(token_type type) const;

    uint32_t get_instruction_parity(uint32_t category_and_type) const;
    uint32_t get_opcode_category_and_type_from_token(const token& tok) const;
  };

}  // namespace other

#endif  // OTHERLIB_VM_COMMAND_FILES_OASM_PARSER_HPP