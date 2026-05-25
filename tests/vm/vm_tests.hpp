/**
 * \file vm/vm_tests.hpp
 **/
#ifndef OTHER_TESTS_VM_TESTS_HPP
#define OTHER_TESTS_VM_TESTS_HPP

#include "vm/command_files/data_block.hpp"
#include "vm/command_files/oasm_parser.hpp"

#include "other_test.hpp"

namespace other {
  namespace detail {

    struct expected_argument {
      token_type type = TOKEN_TYPE_INVALID;
      std::string raw_txt = "";
      std::optional<uint16_t> value = std::nullopt;
      std::vector<uint8_t> raw_data = {};
    };

    struct expected_instruction {
      uint32_t category_and_type = 0;
      std::vector<expected_argument> arguments = {};
    };

    struct expected_data_object {
      std::string name = "";
      std::string value_text = "";
      data_type type = OCMD_DATA_TYPE_INVALID;
      std::vector<uint8_t> data = {};
    };

    struct expected_definition {
      std::string name = "";
      std::string value = "";
    };

    struct register_case {
      std::string_view text;
      token_type type;
      uint16_t value;
    };

    struct generated_data_block {
      std::string source = "";
      std::vector<expected_data_object> objects = {};
    };

    struct generated_code_block {
      std::string name = "";
      std::string source = "";
      std::vector<expected_instruction> instructions = {};
    };
    struct generated_definition {
      std::string name = "";
      std::string value = "";
      std::vector<expected_definition> instructions = {};
    };

    struct generated_program {
      std::string source = "";
      std::string expected_error_substring = "";
      std::vector<generated_data_block> data_blocks = {};
      std::vector<generated_code_block> code_blocks = {};
      std::vector<generated_definition> definitions = {};
    };

    constexpr std::array k_register_cases = {
      register_case{ "r0", TOKEN_TYPE_KW_R0, 0x00 },
      register_case{ "r1", TOKEN_TYPE_KW_R1, 0x01 },
      register_case{ "r2", TOKEN_TYPE_KW_R2, 0x02 },
      register_case{ "r3", TOKEN_TYPE_KW_R3, 0x03 },
      register_case{ "r4", TOKEN_TYPE_KW_R4, 0x04 },
      register_case{ "r5", TOKEN_TYPE_KW_R5, 0x05 },
      register_case{ "r6", TOKEN_TYPE_KW_R6, 0x06 },
      register_case{ "r7", TOKEN_TYPE_KW_R7, 0x07 },
      register_case{ "r8", TOKEN_TYPE_KW_R8, 0x08 },
      register_case{ "r9", TOKEN_TYPE_KW_R9, 0x09 },
      register_case{ "ra", TOKEN_TYPE_KW_RA, 0x0A },
      register_case{ "rb", TOKEN_TYPE_KW_RB, 0x0B },
      register_case{ "rc", TOKEN_TYPE_KW_RC, 0x0C },
      register_case{ "rd", TOKEN_TYPE_KW_RD, 0x0D },
      register_case{ "re", TOKEN_TYPE_KW_RE, 0x0E },
      register_case{ "rf", TOKEN_TYPE_KW_RF, 0x0F },
    };

    template <typename T>
    std::vector<uint8_t> raw_bytes_from_value(const T& value) {
      std::vector<uint8_t> data(sizeof(T));
      std::memcpy(data.data(), &value, sizeof(T));
      return data;
    }

    std::vector<uint8_t> raw_bytes_from_string(const std::string_view text);
    std::string hex_byte_string(const uint8_t value);
    std::string hex_word_string(const uint16_t value);
    ocmd_ir parse_source(const std::string_view source);
    void expect_argument_matches(const raw_instruction::argument& actual, const expected_argument& expected);
    void expect_instruction_matches(const raw_instruction& actual, const expected_instruction& expected);
    void expect_code_block_matches(const code_block& actual, const std::string_view expected_name, const std::span<const expected_instruction> expected_instructions);
    void expect_data_block_matches(const data_block& actual, const std::string_view expected_name, const std::span<const expected_data_object> expected_objects);
    void expect_definition_matches(const compiler_definition& actual, const std::string_view expected_name, const std::string_view expected_value);
    const register_case& get_register_case(const size_t index);
    expected_argument make_register_argument(const register_case& reg);
    expected_argument make_address_argument(const uint16_t value);
    expected_argument make_label_argument(const std::string_view label);
    std::vector<expected_instruction> expected_foo_code_block();

    struct generator {
      std::mt19937 gen;
      std::uniform_int_distribution<size_t> data_block_count_dist;
      std::uniform_int_distribution<size_t> code_block_count_dist;
      std::uniform_int_distribution<size_t> object_count_dist;
      std::uniform_int_distribution<size_t> instruction_count_dist;
      std::uniform_int_distribution<size_t> data_kind_dist;
      std::uniform_int_distribution<size_t> instruction_kind_dist;
      std::uniform_int_distribution<size_t> register_dist;
      std::uniform_int_distribution<uint16_t> address_dist;
      std::uniform_int_distribution<uint16_t> integer_dist;
      std::uniform_int_distribution<int> should_define_dist;
      std::uniform_int_distribution<size_t> definition_dist;
      std::uniform_int_distribution<int> include_end_dist;
      std::uniform_int_distribution<size_t> blob_size_dist;
      std::uniform_int_distribution<int> byte_dist;
      std::uniform_int_distribution<size_t> label_dist;

      generator() : gen(0x0A51F00Du),
                    data_block_count_dist(1, 3),
                    code_block_count_dist(1, 3),
                    object_count_dist(1, 3),
                    instruction_count_dist(1, 3),
                    data_kind_dist(0, 4),
                    instruction_kind_dist(0, 3),
                    register_dist(0, k_register_cases.size() - 1),
                    address_dist(0x0100, 0xFFFE),
                    integer_dist(1, 400),
                    should_define_dist(0, 1),
                    definition_dist(0, 3),
                    include_end_dist(0, 1),
                    blob_size_dist(1, 4),
                    byte_dist(0, 255) {}
    };

    generated_program generate_simple_oasm_program(generator& gen, const size_t iteration);
    generated_program generated_simple_bad_oasm_program(generator& gen, const size_t iteration);

  }  // namespace detail

  class vm_tests : public other_test {
   public:
  };

}  // namespace other

#endif  // OTHER_TESTS_VM_TESTS_HPP