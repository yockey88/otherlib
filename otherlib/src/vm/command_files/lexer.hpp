/**
 * \file vm/command_files/lexer.hpp
 **/
#ifndef OTHERLIB_VM_COMMAND_FILES_LEXER_HPP
#define OTHERLIB_VM_COMMAND_FILES_LEXER_HPP

#include <string>
#include <string_view>
#include <vector>

#include "vm/command_files/token.hpp"
#include "vm/diagnostics/vm_diagnostic.hpp"

namespace other {

  class diagnostic_engine;

  class ocmd_lexer {
   public:
    ocmd_lexer(const std::string_view source_code)
        : source(source_code) {}
    ~ocmd_lexer() = default;

    std::vector<token> tokenize(diagnostic_engine* diag);

   private:
    diagnostic_engine* diagnostics;
    std::vector<token> tokens;

    std::string source;

    std::string current_token;
    size_t current_line = 1;
    size_t current_column = 1;

    source_location current_source_span_start;

    size_t cursor = 0;

    void handle_whitespace();
    void handle_numeric();
    void handle_floating_point();
    void handle_scientific_notation();
    void handle_hexadecimal();
    void handle_alpha();
    void handle_operator();
    void handle_string();
    void handle_comment();

    void add_token(token_type type);

    void newline(bool advance = true);
    void consume();
    void advance();
    void discard_current_token();

    bool finished() const;

    char peek(size_t offset) const;
    char peek_next() const;
    char current() const;

    bool check(char c) const;
    bool check_next(char c) const;

    token_type get_keyword_type(const std::string_view& str) const;
  };

}  // namespace other

#endif  // OTHERLIB_VM_COMMAND_FILES_LEXER_HPP