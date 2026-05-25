/**
 * \file vm/command_files/lexer.cpp
 **/
#include "vm/command_files/lexer.hpp"

#include <algorithm>
#include <ranges>

#include "core/logger.hpp"

#include "vm/command_files/token.hpp"

namespace other {
  namespace {

    struct lex_error : public std::runtime_error {
      lex_error(const std::string& msg)
          : std::runtime_error(msg) {}
    };

  }  // namespace

  static inline bool is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
  }

  static inline bool is_operator_or_punctuation(char c) {
    return std::ranges::find(kOperators, c) != kOperators.end() || std::ranges::find(kPunctuation, c) != kPunctuation.end();
  }

  static inline bool is_numeric(char c) {
    return std::isdigit(static_cast<unsigned char>(c));
  }

  static inline bool is_alpha(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
  }

  static inline bool is_alphanumeric(char c) {
    return is_alpha(c) || is_numeric(c);
  }

  static inline bool is_ocmd_keyword(const std::string_view str) {
    return std::ranges::find(kKeywordTokens, str, &keyword_token::text) != kKeywordTokens.end();
  }

  std::vector<token> ocmd_lexer::tokenize() {
    tokens.emplace_back(TOKEN_TYPE_SOURCE_START, "", 1, 1);
    while (!finished()) {
      try {
        char c = current();
        if (c == '\0') {
          break;
        }

        if (is_whitespace(c)) {
          handle_whitespace();
        } else if (is_numeric(c)) {
          handle_numeric();
        } else if (is_alpha(c)) {
          handle_alpha();
        } else if (is_operator_or_punctuation(c)) {
          handle_operator();
        } else {
          throw lex_error("Unexpected character encountered during lexing");
        }
      } catch (const lex_error& e) {
        CORE_LOG_ERROR("Lexing error at line {}, column {}: {}", current_line, current_column, e.what());
        return {};
      } catch (const std::exception& e) {
        CORE_LOG_ERROR("Unexpected error during lexing at line {}, column {}: {}", current_line, current_column, e.what());
        return {};
      } catch (...) {
        CORE_LOG_ERROR("Unknown error during lexing at line {}, column {}", current_line, current_column);
        return {};
      }
    }

    tokens.emplace_back(TOKEN_TYPE_EOF, "", current_line, current_column);
    return tokens;
  }

  void ocmd_lexer::handle_whitespace() {
    while (is_whitespace(current())) {
      if (check('\n')) {
        newline();
      } else {
        consume();
      }
    }
  }

  void ocmd_lexer::handle_numeric() {
    do {
      advance();
    } while (!finished() && is_numeric(current()));

    if (check('.')) {
      handle_floating_point();
    } else {
      if (check('x') || check('X')) {
        handle_hexadecimal();
      } else if (std::isxdigit(current())) {
        while (!finished() && std::isxdigit(static_cast<unsigned char>(current()))) {
          advance();
        }
        add_token(TOKEN_TYPE_HEX_LITERAL);
      } else {
        add_token(TOKEN_TYPE_INTEGER_LITERAL);
      }
    }
  }

  void ocmd_lexer::handle_floating_point() {
    advance();  // Consume '.'
    if (is_numeric(current())) {
      while (is_numeric(current())) {
        advance();
      }
    }

    if (check('.')) {
      throw lex_error("Multiple decimal points in floating-point literal");
    }

    if (check('e') || check('E')) {
      handle_scientific_notation();
      return;
    }

    if (check('f')) {
      consume();
    }

    add_token(TOKEN_TYPE_FLOATING_POINT_LITERAL);
  }

  void ocmd_lexer::handle_scientific_notation() {
    consume();  // Consume 'e' or 'E'

    bool small = false;

    double lead_digit;
    try {
      lead_digit = std::stod(current_token);
    } catch (std::out_of_range& e) {
      throw lex_error("Leading digit in scientific notation out of range");
    } catch (std::invalid_argument& e) {
      throw lex_error("Leading digit in scientific notation is not a number");
    }

    discard_current_token();

    if (check('-')) {
      consume();
      small = true;
    }

    if (is_numeric(current())) {
      while (is_numeric(current())) {
        advance();
      }
    }

    uint32_t val;
    try {
      val = std::stoi(current_token);
    } catch (std::out_of_range& e) {
      throw lex_error("Scientific notation exponent out of range");
    } catch (std::invalid_argument& e) {
      throw lex_error("Scientific notation exponent is not a number");
    }

    std::string small_str = "0.";
    if (small) {
      auto pos = std::to_string(lead_digit).find('.');
      std::string lead_digit_str = pos == std::string::npos ?
        std::to_string(lead_digit) :
        std::to_string(lead_digit).substr(0, pos) + std::to_string(lead_digit).substr(pos + 1, current_token.size() - pos - 1);

      while (lead_digit_str[lead_digit_str.size() - 1] == '0') {
        lead_digit_str.pop_back();
      }

      for (uint32_t i = 0; i < val - 1; i++) {
        small_str += "0";
      }
      small_str += lead_digit_str;
    }

    std::string result = small ?
      small_str :
      std::to_string(lead_digit * std::pow(10, val));
    current_token = result;

    if (std::stod(current_token) > std::numeric_limits<float>::max()) {
      throw lex_error("Floating-point literal out of range");
    }

    add_token(TOKEN_TYPE_FLOATING_POINT_LITERAL);
  }

  void ocmd_lexer::handle_hexadecimal() {
    advance();  // Consume 'x' or 'X'

    if (!std::isxdigit(static_cast<unsigned char>(current()))) {
      throw lex_error("Invalid hexadecimal literal: expected hexadecimal digits after '0x'");
    }

    while (std::isxdigit(static_cast<unsigned char>(current()))) {
      advance();
    }

    add_token(TOKEN_TYPE_HEX_LITERAL);
  }

  void ocmd_lexer::handle_alpha() {
    while (is_alphanumeric(current()) || current() == '_') {
      advance();
    }

    if (is_ocmd_keyword(current_token)) {
      add_token(get_keyword_type(current_token));
    } else if (std::ranges::all_of(current_token, [](char c) { return std::isxdigit(static_cast<unsigned char>(c)); })) {
      add_token(TOKEN_TYPE_HEX_LITERAL);
    } else {
      add_token(TOKEN_TYPE_IDENTIFIER);
    }
  }

  void ocmd_lexer::handle_operator() {
    char c = current();

    advance();

    switch (c) {
      case '{': add_token(TOKEN_TYPE_LEFT_BRACE); break;
      case '}': add_token(TOKEN_TYPE_RIGHT_BRACE); break;
      case '(': add_token(TOKEN_TYPE_LEFT_PAREN); break;
      case ')': add_token(TOKEN_TYPE_RIGHT_PAREN); break;
      case '[': add_token(TOKEN_TYPE_LEFT_BRACKET); break;
      case ']': add_token(TOKEN_TYPE_RIGHT_BRACKET); break;
      case ',': add_token(TOKEN_TYPE_COMMA); break;
      case '.': add_token(TOKEN_TYPE_DOT); break;
      case '#': add_token(TOKEN_TYPE_HASH); break;
      case '$': add_token(TOKEN_TYPE_DOLLAR); break;
      case '@': add_token(TOKEN_TYPE_AT); break;
      case ':': add_token(TOKEN_TYPE_COLON); break;
      case '=': add_token(TOKEN_TYPE_EQUAL); break;
      case '\'':
      case '"': handle_string(); break;
      case ';': handle_comment(); break;
      case '/': {
        if (check(';')) {
          handle_comment();
        } else {
          add_token(TOKEN_TYPE_SLASH);
        }
      } break;

      // case '+':
      //   if (check('=')) {
      //     advance();
      //     add_token(PLUS_EQUAL);
      //   } else {
      //     add_token(PLUS);
      //   }
      //   break;
      // case '-':
      //   add_token(MINUS);
      //   if (is_numeric(peek())) {
      //     flags.sign = true;
      //   }
      //   break;
      // case '*':
      //   add_token(STAR);
      //   break;
      // case '=':
      //   if (check('=')) {
      //     advance();
      //     add_token(EQUAL_EQUAL);
      //   } else {
      //     add_token(EQUAL_OP);
      //   }
      //   break;
      // case '<':
      //   if (check('=')) {
      //     advance();
      //     add_token(LESS_EQUAL_OP);
      //   } else {
      //     add_token(LESS_OP);
      //   }
      //   break;
      // case '>':
      //   if (check('=')) {
      //     advance();
      //     add_token(GREATER_EQUAL_OP);
      //   } else {
      //     add_token(GREATER_OP);
      //   }
      //   break;
      // case '!':
      //   if (check('=')) {
      //     advance();
      //     add_token(BANG_EQUAL);
      //   } else {
      //     add_token(BANG);
      //   }
      //   break;
      // case '&':
      //   if (Check('&')) {
      //     Advance();
      //     AddToken(LOGICAL_AND);
      //   } else {
      //     throw Error(ShaderError::SYNTAX_ERROR, "Unknown operator : '&'");
      //   }
      //   break;
      // case '|':
      //   if (Check('|')) {
      //     Advance();
      //     AddToken(LOGICAL_OR);
      //   } else {
      //     throw Error(ShaderError::SYNTAX_ERROR, "Unknown operator : '|'");
      //   }
      //   break;
      default:
        throw lex_error("Unknown operator or punctuation character encountered during lexing");
    }
  }

  void ocmd_lexer::handle_string() {
    char qtype = current_token.back();
    current_token.pop_back();  // Remove the quote

    while (!check(qtype)) {
      if (check('\n')) {
        throw lex_error("Unterminated string literal at end of line");
      } else if (finished()) {
        throw lex_error("Unterminated string literal at end of file");
      }
      advance();
    }

    consume();

    add_token(TOKEN_TYPE_STRING_LITERAL);
  }

  void ocmd_lexer::handle_comment() {
    if (check(';')) {
      consume();

      bool found_close = check(';') && check_next('/');
      consume();

      while (!found_close && !finished()) {
        consume();
        found_close = check(';') && check_next('/');
      }

      if (!found_close) {
        throw lex_error("Unterminated block comment");
      }

      consume();
      consume();
    } else {
      while (!check('\n') && !finished()) {
        consume();
      }
    }

    discard_current_token();
  }

  void ocmd_lexer::add_token(token_type type) {
    tokens.emplace_back(type, current_token, current_line, current_column);
    discard_current_token();
  }

  void ocmd_lexer::newline(bool advance) {
    ++current_line;
    current_column = 1;
    if (advance) {
      ++cursor;
    }
  }

  void ocmd_lexer::consume() {
    ++cursor;
    ++current_column;
  }

  void ocmd_lexer::advance() {
    current_token += source[cursor];
    consume();
  }

  void ocmd_lexer::discard_current_token() {
    current_token = "";
  }

  bool ocmd_lexer::finished() const {
    return cursor >= source.size();
  }

  char ocmd_lexer::peek(size_t offset) const {
    if (finished()) {
      return '\0';
    }
    return source[cursor + offset];
  }

  char ocmd_lexer::peek_next() const {
    return peek(1);
  }

  char ocmd_lexer::current() const {
    return peek(0);
  }

  bool ocmd_lexer::check(char c) const {
    return current() == c;
  }

  bool ocmd_lexer::check_next(char c) const {
    return peek_next() == c;
  }

  token_type ocmd_lexer::get_keyword_type(const std::string_view& source_str) const {
    auto it = std::ranges::find(kKeywordTokens, source_str, &keyword_token::text);
    if (it != kKeywordTokens.end()) {
      return it->type;
    } else {
      throw lex_error("Keyword not found for string: " + std::string(source_str));
    }
  }

}  // namespace other