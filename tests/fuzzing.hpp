/**
 * \file tests/fuzzing.hpp
 **/
#ifndef OTHER_TESTS_FUZZING_HPP
#define OTHER_TESTS_FUZZING_HPP

#include <random>

#include "vm/vm_tests.hpp"

namespace other {
  namespace detail {

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

      generator();
    };

    generated_program generate_simple_oasm_program(generator& gen, const size_t iteration);
    generated_program generate_simple_linkable_oasm_program(generator& gen, const size_t iteration);
    generated_program generated_simple_bad_oasm_program(generator& gen, const size_t iteration);

  }  // namespace detail
}  // namespace other

#endif  // OTHER_TESTS_FUZZING_HPP