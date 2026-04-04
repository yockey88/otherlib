/**
 * @file serialization/parser_combinators.cpp
 **/
#include "serialization/parser_combinators.hpp"

#include <cstdio>
#include <ranges>
#include <string>

namespace other {

  std::istream& trim_beginning(std::istream& stream) {
    while (!stream.eof() && std::isspace(stream.peek())) {
      stream.ignore();
    }
    return stream;
  }

  std::string trim_end(const std::string& str) {
    std::string result{ str };
    result.erase(std::find_if(result.rbegin(), result.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), result.end());
    return result;
  }

  std::string trim_beginning_and_end(const std::string& str) {
    std::string res = trim_end(str);
    res.erase(res.begin(), std::find_if(res.begin(), res.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    return res;
  }

  std::string strip_parens(const std::string& str) {
    return str |
      std::views::filter([](char c) { return c != '{' && c != '}'; }) |
      std::views::filter([](char c) { return c != '[' && c != ']'; }) |
      std::views::filter([](char c) { return c != '(' && c != ')'; }) |
      std::ranges::to<std::string>();
  }

  void parse_context::cursor::update(char c) {
    /// TODO: customize this for different tab widths
    auto [o, r, col] = cursor_updater<2>()(offset, { row, column }, c);
    offset = o;
    row = r;
    column = col;
  }

  std::streambuf::int_type parse_context::underflow() {
    return sbuf->sgetc();
  }

  std::streambuf::int_type parse_context::uflow() {
    return last_read = sbuf->sbumpc();
  }
  // Note uflow() is not called for reading out eof.

  std::streampos parse_context::seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which) {
    // Note istream(not streambuf) implements tellg() as seekoff(0, ios_base::cur).
    return sbuf->pubseekoff(off, way, which);
  }

  std::streampos parse_context::seekpos(std::streampos pos, std::ios_base::openmode which) {
    return sbuf->pubseekpos(pos, which);
  }

  char character_parser::operator()(std::istream& stream) const {
    if (stream.fail()) {
      return '\0';
    }

    return update(stream);
  }

  char character_parser::update(std::istream& stream) const {
    if (stream.eof()) {
      stream.setstate(std::ios::failbit);
      return EOF;
    }

    char c = stream.peek();
    if (match(c) && c != '\0') {
      c = stream.get();
      update_stream(stream);
      return c;
    }

    stream.setstate(std::ios::failbit);
    return '\0';
  }
  bool match_character::match(char c) const {
    return c == ch;
  }

  ref<parser<char>> char_parser(char c) {
    return make_ref<match_character>(c);
  }

  bool match_any::match(char c) const {
    return true;
  }

  ref<parser<char>> any_matcher() {
    return make_ref<match_any>();
  }

  bool match_none::match(char c) const {
    return std::isspace(c);
  }

  ref<parser<char>> none_matcher() {
    return make_ref<match_none>();
  }

  bool match_one_of::match(char c) const {
    return chars.find(c) != std::string::npos;
  }

  ref<parser<char>> group_matcher(const std::string_view chars) {
    return make_ref<match_one_of>(chars);
  }

  bool match_none_of::match(char c) const {
    return chars.find(c) == std::string::npos;
  }

  ref<parser<char>> exclude_group(const std::string_view chars) {
    return make_ref<match_none_of>(chars);
  }

  bool match_any_except::match(char c) const {
    return c != ch;
  }

  ref<parser<char>> any_except_matcher(char c) {
    return make_ref<match_any_except>(c);
  }

  bool match_none_except::match(char c) const {
    return c == ch;
  }

  ref<parser<char>> none_except_matcher(char c) {
    return make_ref<match_none_except>(c);
  }

  bool match_function::match(char c) const {
    return func(c);
  }
  std::string parse_string::operator()(std::istream& stream) const {
    if (stream.fail()) {
      return "";
    }

    std::string result;
    MARK_OFFSET(stream);
    for (const auto& c : str) {
      if (stream.peek() == c) {
        result.push_back(stream.get());
        update_stream(stream);
      } else {
        stream.setstate(std::ios::failbit);
        RETURN_OR_WEAK_FAILURE(stream, result);
      }
    }

    return result;
  }

  std::string parse_all_until::operator()(std::istream& stream) const {
    if (stream.fail()) {
      return "";
    }

    std::string result;
    char c = stream.peek();
    while (!chars.contains(c) && !stream.eof()) {
      result.push_back(stream.get());
      update_stream(stream);
      c = stream.peek();
    }
    /// ignores the delimiter character
    if (!stream.eof()) {
      stream.ignore();
    }

    return result;
  }

  std::string parse_all_until_then_take::operator()(std::istream& stream) const {
    if (stream.fail()) {
      return "";
    }

    std::string result;
    char c = stream.peek();
    while (!chars.contains(c) && !stream.eof()) {
      result.push_back(stream.get());
      update_stream(stream);
    }  /// never found token so this is an actuall fatal error, throw here
    if (!stream.eof()) {
      throw parsing_error();
    }

    result.push_back(stream.get());
    update_stream(stream);

    return result;
  }

  ref<parser<void>> skip_spaces() {
    return skip(many<std::string>(match_whitespace()));
  }

  ref<parser<std::string>> parse_until(char c) {
    return make_ref<parse_all_until>(c);
  }

  ref<parser<std::string>> parse_until_then_take(char c) {
    return make_ref<parse_all_until_then_take>(c);
  }

  ref<parser<std::string>> parse_until(const std::string_view chars) {
    return make_ref<parse_all_until>(chars);
  }

  ref<parser<std::string>> parse_until_then_take(const std::string_view chars) {
    return make_ref<parse_all_until_then_take>(chars);
  }

  ref<parser<char>> match_blank() {
    match_function::matcher_fn blank = &std::isblank;
    return make_ref<match_function>(blank);
  }

  ref<parser<char>> match_alpha() {
    match_function::matcher_fn alpha = &std::isalpha;
    return make_ref<match_function>(alpha);
  }

  ref<parser<char>> match_digit() {
    match_function::matcher_fn digit = &std::isdigit;
    return make_ref<match_function>(digit);
  }

  ref<parser<char>> match_alnum() {
    match_function::matcher_fn alnum = &std::isalnum;
    return make_ref<match_function>(alnum);
  }

  ref<parser<char>> match_whitespace() {
    match_function::matcher_fn whitespace = &std::isspace;
    return make_ref<match_function>(whitespace);
  }

  ref<parser<std::string>> match_any_string() {
    return many<std::string>(any_matcher());
  }

  int is_not_whitespace(int c) {
    return !std::isspace(c);
  }

  ref<parser<std::string>> match_any_word() {
    match_function::matcher_fn word = &is_not_whitespace;
    return many<std::string>(make_ref<match_function>(word));
  }

  ref<parser<std::string>> match_any_string_without(const std::string_view chars) {
    return many<std::string>(exclude_group(chars));
  }

  ref<parser<std::string>> match_string(const std::string_view str) {
    return make_ref<parse_string>(str);
  }
  namespace {

    constexpr inline std::pair<std::istream& (*)(std::istream&), std::string (*)(const std::string&)> trim_whitespace{ &trim_beginning, &trim_end };

  }  // anonymous namespace

  ref<parser<std::string>> skip_whitespace_then_match(const std::string_view str) {
    return skip_spaces() >> match_string(str);
  }

  ref<parser<std::string>> match_and_trim(const std::string_view str) {
    return skip_whitespace_then_match(str) | &trim_end;
  }

  ref<parser<std::string>> match_and_strip_parens(const std::string_view str) {
    return match_and_trim(str) | &strip_parens;
  }

  namespace {

    constexpr std::string_view kIdentifierGroup = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-";

  }  // namespace

  ref<parser<std::string>> match_identifier() {
    return match_alpha() >> many<std::string>(group_matcher(kIdentifierGroup));
  }

  ref<parser<std::string>> match_identifier_and_allow(const std::string_view chars) {
    return match_alpha() >> many<std::string>(group_matcher(std::string{ kIdentifierGroup } + std::string{ chars }));
  }

  ref<parser<std::string>> match_identifier_and_strip_parens() {
    auto id_w_parens = skip(group_matcher("({[")) >> match_identifier();
    return (skip_spaces() >> id_w_parens >> str(group_matcher("]})"))) | &strip_parens;
  }

  std::string parse_string_exact::operator()(std::istream& stream) const {
    std::string result = (*parser_obj)(stream);
    if (stream.fail()) {
      throw parsing_error();
    }

    if (stream.eof() || stream.peek() == EOF) {
      return result;
    }

    if (!std::isspace(stream.peek())) {
      throw parsing_error();
    }
    stream.ignore();
    return result;
  }

  ref<parser<std::string>> match_exact(const std::string_view str) {
    return make_ref<parse_string_exact>(str);
  }

  std::string parse_one_of::operator()(std::istream& stream) const {
    if (stream.fail()) {
      return "";
    }

    std::string result;
    result = (*parser_obj)(stream);

    if (std::ranges::find(strings, result) == strings.end()) {
      throw parsing_error();
    }

    return result;
  }

  ref<parser<std::string>> match_any_string_from(const std::vector<std::string>& strings) {
    return make_ref<parse_one_of>(strings);
  }

  ref<parser<std::string>> match_any_string_until_word(const std::string_view word) {
    auto string_until_space = parse_until(' ');
    auto space_then_word = skip_spaces() >> match_string(word);
    return string_until_space | [&space_then_word](const std::string& str) -> std::string {
      std::istringstream stream(str);
      if (stream.eof()) {
        return str;
      }

      try {
        (*space_then_word)(stream);
        if (stream.fail()) {
          throw parsing_error();
        }

        return str.substr(0, stream.tellg());
      } catch (...) {
        return str;
      }
    };
  }

  std::string concat_parser::operator()(std::istream& stream) const {
    if (stream.fail() || stream.eof()) {
      return "";
    }

    std::string result = (*parser1)(stream);
    if (stream.fail() || result.empty()) {
      throw parsing_error();
    }

    result.append((*parser2)(stream));
    if (stream.fail() || result.empty()) {
      throw parsing_error();
    }

    return result;
  }

  std::string char_to_string(char c) {
    return std::string(1, c);
  }

  ref<parser<std::string>> str(const ref<parser<char>>& parser_obj) {
    return parser_obj | &char_to_string;
  }

  ref<parser<std::string>> operator+(const ref<parser<std::string>>& parser1, const ref<parser<std::string>>& parser2) {
    return make_ref<concat_parser>(parser1, parser2);
  }

  ref<parser<std::string>> operator+(const ref<parser<std::string>>& parser1, const ref<parser<char>>& parser2) {
    return parser1 + str(parser2);
  }

  ref<parser<std::string>> operator+(const ref<parser<char>>& parser1, const ref<parser<std::string>>& parser2) {
    return str(parser1) + parser2;
  }

  ref<parser<std::string>> operator+(const ref<parser<char>>& parser1, const ref<parser<char>>& parser2) {
    return str(parser1) + str(parser2);
  }

  ref<parser<std::vector<std::string>>> split_string_on(char delim) {
    ref<parser<std::string>> string_matcher = match_any_string_without(std::string(1, delim)) | trim_whitespace;
    return split<std::vector<std::string>>(string_matcher, char_parser(delim));
  }

  void parse_eof::operator()(std::istream& stream) const {
    if (stream.fail()) {
      return;
    }

    if (!(stream.eof() || stream.peek() == EOF)) {
      stream.setstate(std::ios::failbit);
    }
  }
  ref<parser<void>> end_of_file() {
    return make_ref<parse_eof>();
  }

  ref<parser<void>> skip_any() {
    return skip(any_matcher());
  }

  ref<parser<void>> skip(char c) {
    return skip(char_parser(c));
  }

  void skip_string::operator()(std::istream& stream) const {
    if (stream.fail()) {
      return;
    }

    if (str.empty()) {
      return;
    }

    MARK_OFFSET(stream);
    for (const auto& c : str) {
      if (stream.peek() == c) {
        stream.ignore();
        update_stream(stream);
      } else {
        stream.setstate(std::ios::failbit);
        RETURN_OR_WEAK_FAILURE(stream);
      }
    }
  }

  ref<parser<void>> skip(const std::string_view str) {
    return make_ref<skip_string>(str);
  }

  void skip_all_until::operator()(std::istream& stream) const {
    if (stream.fail()) {
      return;
    }

    while (!stream.eof() && stream.peek() != ch) {
      stream.ignore();
      update_stream(stream);
    }
  }

  ref<parser<void>> skip_until(char c) {
    return make_ref<skip_all_until>(char_parser(c));
  }

  void skip_all_while::operator()(std::istream& stream) const {
    if (stream.fail()) {
      return;
    }

    char c = (*matcher)(stream);
    while (!stream.eof() && c == '\0') {
      stream.ignore();
      update_stream(stream);

      if (stream.eof()) {
        return;
      }

      c = (*matcher)(stream);
    }
  }

  ref<parser<void>> skip_while(const ref<parser<char>>& matcher) {
    return make_ref<skip_all_while>(matcher);
  }

}  // namespace other
