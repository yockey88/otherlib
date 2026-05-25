/**
 * \file vm/lexer_tests.cpp
 **/
#include <array>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "vm/command_files/lexer.hpp"
#include "vm/command_files/token.hpp"
#include "vm/vm_tests.hpp"

namespace other {

  namespace {

    struct expected_token_spec {
      token_type type;
      std::string_view text;
    };

    struct lexeme_case {
      std::string_view source_text;
      std::string_view token_text;
      token_type type;
    };

    constexpr std::array k_fuzz_lexemes = {
      lexeme_case{ "write", "write", TOKEN_TYPE_KW_WRITE },
      lexeme_case{ "load", "load", TOKEN_TYPE_KW_LOAD },
      lexeme_case{ "tag", "tag", TOKEN_TYPE_KW_TAG },
      lexeme_case{ "r1", "r1", TOKEN_TYPE_KW_R1 },
      lexeme_case{ "alpha_01", "alpha_01", TOKEN_TYPE_IDENTIFIER },
      lexeme_case{ "42", "42", TOKEN_TYPE_INTEGER_LITERAL },
      lexeme_case{ "3.5", "3.5", TOKEN_TYPE_FLOATING_POINT_LITERAL },
      lexeme_case{ "0x1f", "0x1f", TOKEN_TYPE_HEX_LITERAL },
      lexeme_case{ "\"hello\"", "hello", TOKEN_TYPE_STRING_LITERAL },
      lexeme_case{ "{", "{", TOKEN_TYPE_LEFT_BRACE },
      lexeme_case{ "}", "}", TOKEN_TYPE_RIGHT_BRACE },
      lexeme_case{ "[", "[", TOKEN_TYPE_LEFT_BRACKET },
      lexeme_case{ "]", "]", TOKEN_TYPE_RIGHT_BRACKET },
      lexeme_case{ ":", ":", TOKEN_TYPE_COLON },
      lexeme_case{ ",", ",", TOKEN_TYPE_COMMA },
      lexeme_case{ "=", "=", TOKEN_TYPE_EQUAL },
    };

    constexpr std::array k_fuzz_separators = {
      std::string_view{ " " },
      std::string_view{ "\t" },
      std::string_view{ "\n" },
      std::string_view{ "  \n  " },
      std::string_view{ "; fuzz line comment\n" },
      std::string_view{ "/; fuzz block comment ;/ " },
      std::string_view{ "/; fuzz block\ncomment ;/\n" },
    };

    constexpr std::array k_invalid_lexemes = {
      std::string_view{ "%" },
      std::string_view{ "+" },
      std::string_view{ "!" },
      std::string_view{ "?" },
      std::string_view{ "~" },
    };

    void expect_tokens(const std::string_view source, const std::span<const expected_token_spec> expected) {
      const auto tokens = ocmd_lexer{ source }.tokenize();
      ASSERT_EQ(tokens.size(), expected.size())
        << std::format("Expected {} tokens, but found {}", expected.size(), tokens.size());

      for (size_t index = 0; index < expected.size(); ++index) {
        EXPECT_EQ(tokens[index].type, expected[index].type)
          << std::format("FAIL @ {} Expected token type: {}, Found token type: {} (token text: {} v. {})", index, expected[index].type, tokens[index].type, expected[index].text, tokens[index].text);
        EXPECT_EQ(tokens[index].text, expected[index].text)
          << std::format("FAIL @ {} Expected token text: '{}', Found token text: '{}'", index, expected[index].text, tokens[index].text);
      }
    }

    std::string random_separator(std::mt19937& generator) {
      std::uniform_int_distribution<size_t> separator_dist(0, k_fuzz_separators.size() - 1);
      return std::string{ k_fuzz_separators[separator_dist(generator)] };
    }

  }  // namespace

  TEST_F(vm_tests, basic_ocmd_lexing) {
    constexpr std::array expected = {
      expected_token_spec{ TOKEN_TYPE_SOURCE_START, "" },
      expected_token_spec{ TOKEN_TYPE_KW_OBJECT, "object" },
      expected_token_spec{ TOKEN_TYPE_IDENTIFIER, "data_bucket" },
      expected_token_spec{ TOKEN_TYPE_EQUAL, "=" },
      expected_token_spec{ TOKEN_TYPE_LEFT_BRACE, "{" },
      expected_token_spec{ TOKEN_TYPE_KW_TAG, "tag" },
      expected_token_spec{ TOKEN_TYPE_COLON, ":" },
      expected_token_spec{ TOKEN_TYPE_STRING_LITERAL, "hello world" },
      expected_token_spec{ TOKEN_TYPE_COMMA, "," },
      expected_token_spec{ TOKEN_TYPE_KW_DATA, "data" },
      expected_token_spec{ TOKEN_TYPE_COLON, ":" },
      expected_token_spec{ TOKEN_TYPE_LEFT_BRACKET, "[" },
      expected_token_spec{ TOKEN_TYPE_INTEGER_LITERAL, "42" },
      expected_token_spec{ TOKEN_TYPE_COMMA, "," },
      expected_token_spec{ TOKEN_TYPE_HEX_LITERAL, "0x1f" },
      expected_token_spec{ TOKEN_TYPE_RIGHT_BRACKET, "]" },
      expected_token_spec{ TOKEN_TYPE_RIGHT_BRACE, "}" },
      expected_token_spec{ TOKEN_TYPE_EOF, "" },
    };

    constexpr std::string_view source = R"(
    object data_bucket = {
      tag: "hello world",
      data: [42, 0x1f]
    }
    )";
    expect_tokens(source, expected);
  }

  TEST_F(vm_tests, ocmd_lexer_ignores_line_and_block_comments) {
    constexpr std::array expected = {
      expected_token_spec{ TOKEN_TYPE_SOURCE_START, "" },
      expected_token_spec{ TOKEN_TYPE_KW_WRITE, "write" },
      expected_token_spec{ TOKEN_TYPE_KW_R1, "r1" },
      expected_token_spec{ TOKEN_TYPE_COMMA, "," },
      expected_token_spec{ TOKEN_TYPE_INTEGER_LITERAL, "1" },
      expected_token_spec{ TOKEN_TYPE_KW_LOAD, "load" },
      expected_token_spec{ TOKEN_TYPE_KW_R2, "r2" },
      expected_token_spec{ TOKEN_TYPE_COMMA, "," },
      expected_token_spec{ TOKEN_TYPE_INTEGER_LITERAL, "2" },
      expected_token_spec{ TOKEN_TYPE_EOF, "" },
    };

    constexpr std::string_view source = R"(
    write r1, 1 ; line comment
    /; block comment
    still in the comment ;/
    load r2, 2
    )";
    expect_tokens(source, expected);
  }

  TEST_F(vm_tests, ocmd_lexer_returns_empty_stream_on_lex_error) {
    const auto tokens = ocmd_lexer{ "tag \"unterminated" }.tokenize();
    EXPECT_TRUE(tokens.empty());
  }

  TEST_F(vm_tests, ocmd_lexer_light_fuzzing_generated_valid_streams) {
    std::mt19937 generator(0x00C0FFEEu);
    std::uniform_int_distribution<size_t> token_count_dist(4, 18);
    std::uniform_int_distribution<size_t> lexeme_dist(0, k_fuzz_lexemes.size() - 1);

    for (size_t iteration = 0; iteration < 64; ++iteration) {
      SCOPED_TRACE(std::string{ "iteration=" } + std::to_string(iteration));

      std::string source = random_separator(generator);
      std::vector<expected_token_spec> expected;

      const size_t token_count = token_count_dist(generator);
      expected.reserve(token_count);

      expected.emplace_back(TOKEN_TYPE_SOURCE_START, "");
      for (size_t token_index = 0; token_index < token_count; ++token_index) {
        const auto& lexeme = k_fuzz_lexemes[lexeme_dist(generator)];
        source += lexeme.source_text;
        source += random_separator(generator);
        expected.emplace_back(lexeme.type, lexeme.token_text);
      }
      expected.emplace_back(TOKEN_TYPE_EOF, "");

      expect_tokens(source, expected);
    }
  }

  TEST_F(vm_tests, ocmd_lexer_light_fuzzing_rejects_invalid_symbols) {
    std::mt19937 generator(0x00123456u);
    std::uniform_int_distribution<size_t> prefix_count_dist(1, 8);
    std::uniform_int_distribution<size_t> lexeme_dist(0, k_fuzz_lexemes.size() - 1);
    std::uniform_int_distribution<size_t> invalid_dist(0, k_invalid_lexemes.size() - 1);

    for (size_t iteration = 0; iteration < 48; ++iteration) {
      SCOPED_TRACE(std::string{ "iteration=" } + std::to_string(iteration));

      std::string source;
      const size_t prefix_count = prefix_count_dist(generator);

      for (size_t token_index = 0; token_index < prefix_count; ++token_index) {
        const auto& lexeme = k_fuzz_lexemes[lexeme_dist(generator)];
        source += lexeme.source_text;
        source += random_separator(generator);
      }

      source += k_invalid_lexemes[invalid_dist(generator)];
      source += random_separator(generator);
      source += k_fuzz_lexemes[lexeme_dist(generator)].source_text;

      const auto tokens = ocmd_lexer{ source }.tokenize();
      EXPECT_TRUE(tokens.empty())
        << std::format("Expected lexer to fail on invalid input, but it produced {} tokens. Source: '{}'", tokens.size(), source);
    }
  }

}  // namespace other