/**
 * \file vm/command_files/parser.hpp
 **/
#ifndef OTHERLIB_VM_COMMAND_FILES_PARSER_HPP
#define OTHERLIB_VM_COMMAND_FILES_PARSER_HPP

#include <map>
#include <vector>

#include "core/defines.hpp"

#include "vm/command_files/code_block.hpp"
#include "vm/command_files/data_block.hpp"
#include "vm/command_files/token.hpp"

namespace other {

  struct ocmd_ir {
    std::map<natural_t, code_block> code_blocks = {};
    std::map<natural_t, data_block> data_blocks = {};
  };

  class ocmd_parser {
   public:
    ocmd_parser(const std::vector<token>& tokens)
        : tokens(tokens) {}
    ~ocmd_parser() = default;

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

    struct block_section_ir {
      std::vector<code_section_ir> sections = {};
      std::vector<data_section_ir> data_sections = {};
    };
    ocmd_ir ir_result;

    std::vector<token> tokens;

    size_t cursor = 0;

    block_section_ir parse_sections();
    code_section_ir parse_code_block();
    data_section_ir parse_data_block();

    void process_code_sections(std::vector<code_section_ir>& sections);
    void process_data_sections(std::vector<data_section_ir>& sections);

    bool is_type_keyword(const token& tok) const;
    bool is_instruction_keyword(const token& tok) const;
    bool is_eol_marker(const token& tok) const;
    // bool is_return_instruction(uint32_t category_and_type) const;

    const token& peek(size_t offset) const;
    const token& current() const;
    const std::span<const token> look_from_now(size_t count = 0) const;

    void consume();

    bool finished() const;

    bool check(token_type type) const;
    bool check_next(token_type type) const;

    uint32_t get_opcode_category_and_type_from_token(const token& tok) const;
  };

}  // namespace other

#endif  // OTHERLIB_VM_COMMAND_FILES_PARSER_HPP