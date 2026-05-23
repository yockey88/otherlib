/**
 * \file tests/vm/parsing_tests.cpp
 **/
#include "serialization/parser_combinators.hpp"

#include "vm/vm_tests.hpp"

namespace other {

  struct resource_data {
    std::string type;
    std::string path;
  };

  struct output_robject {
    std::string type_name;
    std::string name;
    resource_data data;
  };

  // -- whitespace + comments -----------------------------------------------

  // /// `-- this is a line comment`
  ref<parser<void>> line_comment() {
    return skip(match_string("--")) >> skip_until('\n');
  }

  /// `/-- block comment --/`
  ref<parser<void>> block_comment() {
    return skip(match_string("/--")) >> skip(parse_until_then_take("--/"));
  }

  /// whitespace OR a comment, any number of times
  ref<parser<void>> ws() {
    return skip<std::string>(str(match_whitespace()) | match_string("--") | match_string("/--"));
    // (in practice we wrap individual atoms with skip_whitespace_then_match;
    //  this is the heavy-duty version used between top-level items.)
  }

  // // -- identifiers + literals ----------------------------------------------

  // ref<parser<std::string>> ident() {
  //   return skip_whitespace_then_match("") >> match_identifier();
  //   // identifiers are letters/digits/underscore; engine helper.
  // }

  // ref<parser<std::string>> kw(std::string_view word) {
  //   return skip_whitespace_then_match(std::string(word));
  // }

  // ref<parser<std::string>> quoted_string() {
  //   return skip(skip_spaces()) >> skip(char_parser('"')) >> parse_until_then_take('"') | [](std::string s) {  // drop the trailing quote
  //     if (!s.empty()) s.pop_back();
  //     return s;
  //   };
  // }

  // ref<parser<int>> integer() {
  //   return one_or_more<std::string>(match_digit()) | [](const std::string& s) {
  //     return std::stoi(s);
  //   };
  // }

  // ref<parser<float>> floating() {
  //   auto digits = one_or_more<std::string>(match_digit());
  //   auto fractional = char_parser('.') + digits;
  //   return (digits + maybe(fractional) | [](auto pair) -> std::string {              /* flatten optional */
  //                                                                       return pair; /* str + opt<str> already concatenated by + */
  //          }) |
  //     [](const std::string& s) {
  //       return std::stof(s);
  //     };
  // }

  // tag: `@material` -> resource_tag::from("material")
  // ref<parser<resource_tag>> tag_literal() {
  //   ref<std::string> r = (skip(char_parser('@')) >> ident());
  //   return (r | [](const std::string& name) {
  //     return resource_tag::from(name);
  //   });
  // }

  TEST_F(vm_tests, basic_parsing) {
    std::string source = R"(
      texture "my_texture" := "assets/textures/my_texture.png";
      shader "my_shader" {
        vertex-path: "",
        fragment-path: "",
      }
    )";
  }

}  // namespace other