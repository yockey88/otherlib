/**
 * @file serialization/parser_combinators.hpp
 **/
#ifndef OTHER_CORE_SERIALIZATION_PARSER_COMBINATORS_HPP
#define OTHER_CORE_SERIALIZATION_PARSER_COMBINATORS_HPP

#include <concepts>
#include <istream>
#include <sstream>
#include <streambuf>
#include <string>
#include <string_view>
#include <utility>

// #include "core/meta.hpp"
#include "core/ref.hpp"
#include "core/ref_counted.hpp"
#include "serialization/reflection.hpp"

namespace other {

  template <typename T1, typename T2>
  concept makeable_from =
    requires(T1 t1, T2 t2) { { std::declval<T2>() } -> std::constructible_from<T1, T2>; } ||
    requires(T1 t1, T2 t2) { { T1{t2} }; } ||
    requires(T1 t1, T2 t2) { { t1.append(t2) }; } ||
    requires(T1 t1, T2 t2) { { t1.insert(t1.end(), t2) }; } ||
    requires(T1 t1, T2 t2) { { t1.insert(t1.end(), t2.begin(), t2.end()) }; };

  std::istream& trim_beginning(std::istream& stream);
  std::string trim_end(const std::string& str);
  std::string trim_beginning_and_end(const std::string& str);
  std::string strip_parens(const std::string& str);

  class parse_context : public std::streambuf {
   public:
    parse_context(std::streambuf* sbuf) : sbuf(sbuf) {}

   protected:
    std::streambuf* const sbuf;

    std::streambuf::int_type underflow();
    std::streambuf::int_type uflow();

    std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out);
    std::streampos seekpos(std::streampos pos, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out);

   private:
    template <size_t TW>
    struct cursor_updater {
      std::tuple<std::streamoff, size_t, size_t> operator()(std::streamoff offset, std::pair<size_t, size_t> cursor, char c) {
        cursor.first++;
        if (c == '\t') {
          cursor.second += TW;
        } else {
          cursor.second++;
        }
        return { offset + 1, cursor.first, cursor.second };
      }
    };

   public:
    struct cursor {
      std::streamoff offset;
      size_t row = 1, column = 1;
      void update(char c);
    } curs;

    char last_read = 0;
  };

  struct parsing_error {};

  /// TODO: make this error handling more robust and add information about the error to the exception

#define MARK_OFFSET(stream) std::streamoff _off = stream.tellg()

#define CREATE_MARK(mark, stream) std::streamoff mark = stream.tellg();

#define GOTO_MARK(mark, stream) stream.seekg(mark, std::ios::beg);

#define RETURN_TO_MARK(stream)              \
  do {                                      \
    stream.clear();                         \
    stream.seekg(_off, std::ios_base::beg); \
  } while (false)

#define CHECK_STREAM(stream)             \
  if (stream.fail() && stream.tellg()) { \
    throw parsing_error();               \
  } else

#define RETURN_OR_WEAK_FAILURE(stream, ...)     \
  do {                                          \
    if (stream.fail() && stream.tellg() == 0) { \
      throw parsing_error();                    \
    }                                           \
    return __VA_ARGS__;                         \
  } while (0)

#define RETURN_IF_FAIL(stream, ...) \
  if (stream.fail()) {              \
    if (stream.tellg() == 0) {      \
      throw parsing_error();        \
    }                               \
    return __VA_ARGS__;             \
  } else

  // typing savers for static_cast<pos_stream *>(s.rdbuf())
  inline void update_stream(std::istream& stream) {
    parse_context* parse_stream = static_cast<parse_context*>(stream.rdbuf());
    parse_stream->curs.update(parse_stream->last_read);
  }

  inline parse_context::cursor& stream_position(std::istream& s) {
    return static_cast<parse_context*>(s.rdbuf())->curs;
  }

  template <typename T>
  struct parser : public ref_counted {
    parser() = default;

    virtual ~parser() = default;
    virtual T operator()(std::istream& stream) const = 0;
  };

  template <typename T>
  inline T operator>>(std::istream& stream, const ref<parser<T>>& parser_obj) {
    return (*parser_obj)(stream);
  }

  template <>
  inline void operator>>(std::istream& stream, const ref<parser<void>>& parser_obj) {
    (*parser_obj)(stream);
  }

  struct character_parser : parser<char> {
    virtual ~character_parser() = default;
    char operator()(std::istream& stream) const override;

   protected:
    virtual char update(std::istream& stream) const;
    virtual bool match(char c) const = 0;
  };
  struct match_character : character_parser {
    const char ch;

    match_character(char c)
        : ch(c) {}

    bool match(char c) const override;
  };
  ref<parser<char>> char_parser(char c);

  struct match_any : character_parser {
    bool match(char c) const override;
  };
  ref<parser<char>> any_matcher();

  struct match_none : character_parser {
    bool match(char c) const override;
  };
  ref<parser<char>> none_matcher();

  struct match_one_of : character_parser {
    const std::string chars;

    match_one_of(const std::string_view chars)
        : chars(chars) {}

    bool match(char c) const override;
  };
  ref<parser<char>> group_matcher(const std::string_view chars);

  struct match_none_of : character_parser {
    const std::string chars;

    match_none_of(const std::string_view chars)
        : chars(chars) {}

    bool match(char c) const override;
  };
  ref<parser<char>> exclude_group(const std::string_view chars);
  struct match_any_except : character_parser {
    const char ch;

    match_any_except(char c)
        : ch(c) {}

    bool match(char c) const override;
  };
  ref<parser<char>> any_except_matcher(char c);

  struct match_none_except : character_parser {
    const char ch;

    match_none_except(char c)
        : ch(c) {}

    bool match(char c) const override;
  };
  ref<parser<char>> none_except_matcher(char c);

  struct match_function : character_parser {
    using matcher_fn = int (*const)(int);
    matcher_fn func;

    match_function(matcher_fn fn)
        : func(fn) {}

    bool match(char c) const override;
  };

  struct parse_string : parser<std::string> {
    const std::string str;

    parse_string(const std::string_view str)
        : str(str) {}

    std::string operator()(std::istream& stream) const override;
  };

  struct parse_all_until : parser<std::string> {
    std::string chars;

    parse_all_until(char c)
        : chars({ c }) {}

    parse_all_until(const std::string_view chars)
        : chars(chars) {}

    std::string operator()(std::istream& stream) const override;
  };
  struct parse_all_until_then_take : parser<std::string> {
    std::string chars;

    parse_all_until_then_take(char c)
        : chars({ c }) {}

    parse_all_until_then_take(const std::string_view chars)
        : chars(chars) {}

    std::string operator()(std::istream& stream) const override;
  };

  ref<parser<void>> skip_spaces();

  ref<parser<std::string>> parse_until(char c);
  ref<parser<std::string>> parse_until_then_take(char c);
  ref<parser<std::string>> parse_until(const std::string_view chars);
  ref<parser<std::string>> parse_until_then_take(const std::string_view chars);

  ref<parser<char>> match_blank();
  ref<parser<char>> match_alpha();
  ref<parser<char>> match_digit();
  ref<parser<char>> match_alnum();
  ref<parser<char>> match_whitespace();
  ref<parser<std::string>> match_any_string();
  ref<parser<std::string>> match_any_word();
  ref<parser<std::string>> match_any_string_without(const std::string_view chars);
  ref<parser<std::string>> match_string(const std::string_view str);

  ref<parser<std::string>> skip_whitespace_then_match(const std::string_view str);
  ref<parser<std::string>> match_and_trim(const std::string_view str);
  ref<parser<std::string>> match_and_strip_parens(const std::string_view str);
  ref<parser<std::string>> match_identifier();
  ref<parser<std::string>> match_identifier_and_allow(const std::string_view chars);
  ref<parser<std::string>> match_identifier_and_strip_parens();

  struct parse_string_exact : parser<std::string> {
    ref<parser<std::string>> parser_obj;
    parse_string_exact(const std::string_view str)
        : parser_obj(match_string(str)) {}

    std::string operator()(std::istream& stream) const override;
  };
  ref<parser<std::string>> match_exact(const std::string_view str);

  struct parse_one_of : parser<std::string> {
    const std::vector<std::string> strings;
    ref<parser<std::string>> parser_obj;

    parse_one_of(const std::vector<std::string>& strings)
        : strings(strings), parser_obj(match_any_word()) {}

    std::string operator()(std::istream& stream) const override;
  };

  ref<parser<std::string>> match_any_string_from(const std::vector<std::string>& strings);
  ref<parser<std::string>> match_any_string_until_word(const std::string_view word);

  struct parse_eof : parser<void> {
    void operator()(std::istream& stream) const override;
  };

  ref<parser<void>> end_of_file();

  template <typename T>
  struct skip_parser : parser<void> {
    const ref<parser<T>> matcher;

    skip_parser(const ref<parser<T>>& matcher)
        : matcher(matcher) {}

    void operator()(std::istream& stream) const override {
      (void)(*matcher)(stream);  /// matcher will iterate stream if need be
    }
  };

  ref<parser<void>> skip_any();
  template <typename T>
  ref<parser<void>> skip(const ref<parser<T>>& matcher) {
    return make_ref<skip_parser<T>>(matcher);
  }
  ref<parser<void>> skip(char c);

  struct skip_string : parser<void> {
    const std::string str;

    skip_string(const std::string_view str)
        : str(str) {}

    void operator()(std::istream& stream) const override;
  };

  ref<parser<void>> skip(const std::string_view str);

  struct skip_all_until : parser<void> {
    char ch;

    skip_all_until(char c)
        : ch(c) {}

    void operator()(std::istream& stream) const override;
  };

  ref<parser<void>> skip_until(char c);

  struct skip_all_while : parser<void> {
    const ref<parser<char>> matcher;

    skip_all_while(const ref<parser<char>>& matcher)
        : matcher(matcher) {}

    void operator()(std::istream& stream) const override;
  };

  ref<parser<void>> skip_while(const ref<parser<char>>& matcher);

  template <typename T1, typename F>
  concept parser_map_fn =
    requires(const parser<T1>& p1, F map) {
      { p1(std::declval<std::istream&>()) } -> std::same_as<T1>;
      { map(std::declval<T1>()) } -> std::convertible_to<std::invoke_result_t<F, T1>>;
    };

  template <typename T1, typename F>
    requires parser_map_fn<T1, F>
  struct parser_map : parser<std::invoke_result_t<F, T1>> {
    using R = std::invoke_result_t<F, T1>;

    const ref<parser<T1>> parser_obj;
    F map;

    parser_map(const ref<parser<T1>>& parser_obj, F map)
        : parser_obj(parser_obj), map(map) {}

    R operator()(std::istream& stream) const override {
      T1 val = (*parser_obj)(stream);
      if (stream.fail()) {
        return R{};
      }
      return map(val);
    }
  };

  template <typename F>
  struct parser_map<void, F> : parser<std::invoke_result_t<F>> {
    using R = std::invoke_result_t<F>;

    const ref<parser<void>> parser_obj;
    F map;

    parser_map(const ref<parser<void>>& parser_obj, F map)
        : parser_obj(parser_obj), map(map) {}

    R operator()(std::istream& stream) const override {
      (*parser_obj)(stream);
      if (stream.fail()) {
        return R{};
      }
      return map();
    }
  };
  template <typename T1, typename F>
    requires parser_map_fn<T1, F>
  inline ref<parser<std::invoke_result_t<F, T1>>> operator|(const ref<parser<T1>>& parser, F map) {
    return make_ref<parser_map<T1, F>>(parser, map);
  }

  struct concat_parser : parser<std::string> {
    const ref<parser<std::string>> parser1;
    const ref<parser<std::string>> parser2;

    concat_parser(const ref<parser<std::string>>& parser1, const ref<parser<std::string>>& parser2)
        : parser1(parser1), parser2(parser2) {}

    std::string operator()(std::istream& stream) const override;
  };

  std::string char_to_string(char c);
  ref<parser<std::string>> str(const ref<parser<char>>& parser_obj);

  ref<parser<std::string>> operator+(const ref<parser<std::string>>& parser1, const ref<parser<std::string>>& parser2);
  ref<parser<std::string>> operator+(const ref<parser<std::string>>& parser1, const ref<parser<char>>& parser2);
  ref<parser<std::string>> operator+(const ref<parser<char>>& parser1, const ref<parser<std::string>>& parser2);
  ref<parser<std::string>> operator+(const ref<parser<char>>& parser1, const ref<parser<char>>& parser2);
  template <typename CT>
    requires is_container_type<CT>
  struct parse_many : parser<CT> {
    using val_t = typename CT::value_type;

    /// apply this parser many times
    const ref<parser<val_t>> p;

    parse_many(const ref<parser<val_t>>& parser)
        : p(parser) {}

    CT operator()(std::istream& stream) const override {
      if (stream.fail()) {
        return CT{};
      } else if (stream.eof()) {
        stream.setstate(std::ios::failbit);
        return CT{};
      }

      CT result;
      while (!stream.eof() && stream.peek() != '\0') {
        val_t val = (*p)(stream);
        if (stream.fail()) {
          stream.clear();
          /// force copy
          return CT{ result };
        }

        result.insert(result.end(), val);
      }

      if (stream.eof()) {
        stream.setstate(std::ios::failbit);
      }

      return result;
    }
  };

  template <typename T>
    requires(!is_container_type<T>)
  struct skip_many : parser<void> {
    const ref<parser<void>> p;

    skip_many(const ref<parser<T>>& parser)
        : p(parser) {}

    void operator()(std::istream& stream) const {
      while (!stream.eof()) {
        (*p)(stream);
        if (stream.fail()) {
          stream.clear();
          return;
        }
      }
    }
  };

  template <typename CT>
  ref<parser<CT>> many(const ref<parser<typename CT::value_type>>& parser) {
    return make_ref<parse_many<CT>>(parser);
  }

  template <typename T>
    requires(is_container_type<T> || std::is_same_v<T, std::string>)
  struct parse_one_or_more : parser<T> {
    const ref<parser<typename T::value_type>> p;
    parse_one_or_more(const ref<parser<typename T::value_type>>& parser)
        : p(parser) {}

    T operator()(std::istream& stream) const override {
      using val_t = typename T::value_type;

      T result;
      while (!stream.eof()) {
        val_t val = (*p)(stream);
        if (stream.fail()) {
          stream.clear();
          if (result.empty()) {
            return T{};
          }
          return T{ result };
        }

        result.insert(result.end(), val);
      }
      return result;
    }
  };

  template <typename T>
    requires(is_container_type<T> || std::is_same_v<T, std::string>)
  ref<parser<T>> one_or_more(const ref<parser<typename T::value_type>>& parser) {
    return make_ref<parse_one_or_more<T>>(parser);
  }

  template <typename T1, typename T2>
  struct parser_sequence : parser<T2> {
    const ref<parser<T1>> parser1;
    const ref<parser<T2>> parser2;

    parser_sequence(const ref<parser<T1>>& parser1, const ref<parser<T2>>& parser2)
        : parser1(parser1), parser2(parser2) {}

    T2 operator()(std::istream& stream) const override {
      MARK_OFFSET(stream);

      if constexpr (std::same_as<void, T1>) {
        (*parser1)(stream);
        if (stream.fail()) {
          return T2{};
        }

        T2 val = (*parser2)(stream);
        RETURN_OR_WEAK_FAILURE(stream, val);
      } else if constexpr (std::same_as<void, T2>) {
        T1 val = (*parser1)(stream);
        if (stream.fail()) {
          return T2{};
        }

        (*parser2)(stream);
        RETURN_OR_WEAK_FAILURE(stream, T2{ val });
      } else if constexpr (is_container_type<T2> || std::same_as<T2, std::string>) {
        if constexpr (std::same_as<T2, std::string>) {
          static_assert(std::same_as<T1, char> || std::same_as<T1, std::string>, "Cannot append different types to string");
          T2 val = T2{ (*parser1)(stream) };
          if (stream.fail()) {
            return T2{};
          }

          T2 res = (*parser2)(stream);
          val.append(res);

          RETURN_OR_WEAK_FAILURE(stream, val);
        } else if (is_container_type<T2>) {
          static_assert(std::same_as<T1, typename T2::value_type>, "Cannot append different types to container");
          T2 val = T2{ (*parser1)(stream) };
          if (stream.fail()) {
            return T2{};
          }

          T2 res = (*parser2)(stream);
          val.insert(val.end(), res.begin(), res.end());

          RETURN_OR_WEAK_FAILURE(stream, val);
        } else {
          T1 val = (*parser1)(stream);
          if (stream.fail()) {
            return T2{};
          }

          T2 res = (*parser2)(stream);
          RETURN_OR_WEAK_FAILURE(stream, res);
        }
      } else if constexpr (
        requires { { T2{ std::declval<T1>() } }; { std::declval<T2>().append(std::declval<T1>()) }; } ||
        requires { { T2{ std::declval<T1>() } }; { std::declval<T2>().insert(std::declval<T2>().end(), std::declval<T1>()) }; } ||
        requires { { T2{ std::declval<T1>() } }; { std::declval<T2>().insert(std::declval<T2>().end(), std::declval<T2>().begin(), std::declval<T2>().end()) }; }
      ) {
        T2 val = T2{ (*parser1)(stream) };
        if (stream.fail()) {
          return T2{};
        }

        T2 res = (*parser2)(stream);
        if constexpr (requires { val.append(res); }) {
          val.append(res);
        } else if constexpr (requires { val.insert(val.end(), res.begin(), res.end()); }) {
          val.insert(val.end(), res.begin(), res.end());
        } else {
          val.insert(val.end(), res.begin(), res.end());
        }
        return val;
      } else {
        T1 val = (*parser1)(stream);
        if (stream.fail()) {
          return T2{};
        }

        T2 res = (*parser2)(stream);
        RETURN_OR_WEAK_FAILURE(stream, res);
      }
    }
  };
  template <>
  struct parser_sequence<void, void> : parser<void> {
    const ref<parser<void>> parser1;
    const ref<parser<void>> parser2;

    parser_sequence(const ref<parser<void>>& parser1, const ref<parser<void>>& parser2)
        : parser1(parser1), parser2(parser2) {}

    void operator()(std::istream& stream) const override {
      (*parser1)(stream);
      (*parser2)(stream);
    }
  };
  template <typename T1, typename T2>
  inline ref<parser<T2>> seq(const ref<parser<T1>>& parser1, const ref<parser<T2>>& parser2) {
    return make_ref<parser_sequence<T1, T2>>(parser1, parser2);
  }
  template <typename T1, typename T2>
  inline ref<parser<T2>> operator>>(const ref<parser<T1>>& parser1, const ref<parser<T2>>& parser2) {
    return make_ref<parser_sequence<T1, T2>>(parser1, parser2);
  }
  template <typename T>
  concept not_void = !std::same_as<T, void>;

  template <typename... Ts>
  concept none_void = (not_void<Ts> && ...);

  template <typename F, typename T>
  concept function_invocable_with_parser = requires(F f, ref<parser<T>> p) {
    f(p);
  };

  template <typename F, typename... Ts>
  concept applicable_to_all = (function_invocable_with_parser<F, Ts> && ...);

  template <typename... Ts>
  using parser_tuple = std::tuple<ref<parser<Ts>>...>;

  template <size_t I = 0, typename F, typename... Ts>
    requires(I == sizeof...(Ts))
  decltype(auto) apply(const parser_tuple<Ts...>&, F func) {
    return std::make_tuple();
  }

  template <size_t I = 0, typename F, typename... Ts>
    requires(I < sizeof...(Ts))
  decltype(auto) apply(const parser_tuple<Ts...>& parsers, F func) {
    auto argi = std::make_tuple(func(std::get<I>(parsers)));
    auto rest = apply<I + 1, F, Ts...>(parsers, func);
    return std::tuple_cat(argi, rest);
  }
  template <typename... Ts>
  struct multi_parser : parser<std::tuple<Ts...>> {
    const parser_tuple<Ts...> parsers;

    multi_parser(const ref<parser<Ts>>&... parsers)
        : parsers(std::make_tuple(parsers...)) {}

    std::tuple<Ts...> operator()(std::istream& stream) const override {
      auto evaluate = [&stream](auto& parser_obj) -> decltype((*parser_obj)(stream)) {
        return (*parser_obj)(stream);
      };
      return apply<0, decltype(evaluate), Ts...>(parsers, evaluate);
    }
  };
  template <typename... Ts>
  ref<parser<std::tuple<Ts...>>> parse_multiple(const ref<parser<Ts>>&... parsers) {
    return make_ref<multi_parser<Ts...>>(parsers...);
  }
  template <typename C, typename T>
    requires is_container_type<C> && is_container_type<typename C::value_type> && std::same_as<typename C::value_type::value_type, T>
  struct split_on_delimiter : public parser<C> {
    using val_t = typename C::value_type;

    const ref<parser<val_t>> val_parser;
    const ref<parser<T>> delim_parser;

    split_on_delimiter(const ref<parser<val_t>>& val_parser, const ref<parser<T>>& delim_parser)
        : val_parser(val_parser), delim_parser(delim_parser) {}

    C operator()(std::istream& stream) const override {
      C result{};
      val_t val = (*val_parser)(stream);
      if (stream.fail()) {
        stream.clear();
        return result;
      }

      result.insert(result.end(), val);
      do {
        (*delim_parser)(stream);
        if (stream.fail()) {
          stream.clear();
          return result;
        }

        val = (*val_parser)(stream);
        if (stream.fail()) {
          stream.clear();
          return result;
        }

        result.insert(result.end(), val);
      } while (true);

      stream.clear();
      return result;
    }
  };
  template <typename C, typename T>
    requires is_container_type<C> && is_container_type<typename C::value_type> && std::same_as<typename C::value_type::value_type, T>
  ref<parser<C>> split(const ref<parser<typename C::value_type>>& val_parser, const ref<parser<T>>& delim_parser) {
    return make_ref<split_on_delimiter<C, T>>(val_parser, delim_parser);
  }

  ref<parser<std::vector<std::string>>> split_string_on(char delim);

  template <typename T1, typename T2>
  concept both_void = std::same_as<T1, void> && std::same_as<T2, void>;

  template <typename T1, typename T2>
  concept equivalent_parsers =
    makeable_from<T1, T2> &&
    requires(const parser<T1>& p1, const parser<T2>& p2) {
      { p1(std::declval<std::istream&>()) } -> std::convertible_to<T1>;
      { p2(std::declval<std::istream&>()) } -> std::convertible_to<T2>;
    };

  template <typename T1, typename T2>
  concept replacable_with = equivalent_parsers<T1, T2>;

  template <typename T1, typename T2>
    requires replacable_with<T1, T2> || both_void<T1, T2>
  struct parse_or : parser<T1> {
    const ref<parser<T1>> parser_obj;
    const ref<parser<T2>> fallback;

    parse_or(const ref<parser<T1>>& parser_obj, const ref<parser<T2>>& fallback)
        : parser_obj(parser_obj), fallback(fallback) {}

    T1 operator()(std::istream& stream) const override {
      std::streampos pos = stream.tellg();
      if (stream.fail()) {
        return T1{};
      }
      try {
        T1 val = (*parser_obj)(stream);
        if (!stream.fail()) {
          return val;
        } else if (stream.eof()) {
          return T1{};
        }
      } catch (const parsing_error& e) {
        // no-op
      }
      stream.clear();

      try {
        T2 val2 = (*fallback)(stream);
        if (!stream.fail()) {
          return T1{ val2 };
        } else if (stream.eof()) {
          return T1{};
        }
      } catch (const parsing_error& e) {
        // no-op
      }

      /// we reset in case a parser will catch this above and discard the error
      stream.clear();
      stream.seekg(pos, std::ios::beg);

      throw parsing_error();
    }
  };

  template <>
  struct parse_or<void, void> : parser<void> {
    const ref<parser<void>> parser_obj;
    const ref<parser<void>> fallback;

    parse_or(const ref<parser<void>>& parser_obj, const ref<parser<void>>& fallback)
        : parser_obj(parser_obj), fallback(fallback) {}
    void operator()(std::istream& stream) const override {
      if (stream.fail()) {
        return;
      }

      (*parser_obj)(stream);
      if (!stream.fail()) {
        return;
      }

      stream.clear();

      (*fallback)(stream);
      if (stream.fail()) {
        throw parsing_error();
      }

      return;
    }
  };

  template <typename T1, typename T2>
    requires replacable_with<T1, T2>
  ref<parser<T2>> parse_or_fn(const ref<parser<T1>>& parser_obj, const ref<parser<T2>>& fallback) {
    return make_ref<parse_or<T1, T2>>(parser_obj, fallback);
  }

  template <typename T1, typename T2>
    requires replacable_with<T1, T2>
  ref<parser<T1>> operator|(const ref<parser<T1>>& parser_obj, const ref<parser<T2>>& fallback) {
    return parse_or_fn(parser_obj, fallback);
  }

  static inline ref<parser<std::string>> operator|(const ref<parser<std::string>>& parser_obj, const ref<parser<char>>& fallback) {
    return make_ref<parse_or<std::string, std::string>>(parser_obj, str(fallback));
  }

  template <typename T1, typename F>
  concept parser_filter_fn =
    requires(const parser<T1>& p1, F filter) {
      { filter(std::declval<std::istream&>()) } -> std::same_as<std::istream&>;
      { p1(filter(std::declval<std::istream&>())) } -> std::same_as<T1>;
    };

  template <typename T, typename F>
  struct filter_then : parser<T> {
    const ref<parser<T>> parser_obj;
    F filter;

    filter_then(const ref<parser<T>>& parser_obj, F filter)
        : parser_obj(parser_obj), filter(filter) {}
    T operator()(std::istream& stream) const override {
      std::istream& in = filter(stream);

      T val = (*parser_obj)(in);
      if (stream.fail()) {
        return T{};
      }

      return val;
    }
  };
  template <typename T, typename F>
    requires parser_filter_fn<T, F>
  ref<parser<T>> operator|(F filter, const ref<parser<T>>& parser_obj) {
    return make_ref<filter_then<T, F>>(parser_obj, filter);
  }

  template <typename T, typename F, typename M>
    requires parser_filter_fn<T, F> && parser_map_fn<T, M>
  ref<parser<T>> filter_then_map(const ref<parser<T>>& parser_obj, F filter, M map) {
    auto filt = filter | parser_obj;
    return filt | map;
  }

  template <typename T, typename F, typename M>
    requires parser_filter_fn<T, F> && parser_map_fn<T, M>
  ref<parser<std::invoke_result_t<M, T>>> operator|(const ref<parser<T>>& parser_obj, std::pair<F, M> filter_map) {
    return filter_then_map(parser_obj, filter_map.first, filter_map.second);
  }
  template <typename C1, typename C2>
  concept similar_containers = is_container_type<C1> && is_container_type<C2> && std::same_as<typename C1::value_type, typename C2::value_type>;

  template <typename C, typename C1, typename C2>
  concept nestable_parsers = similar_containers<C1, C2> && is_container_type<C> && std::same_as<typename C::value_type, C2>;
  template <typename OC, typename C1, typename C2>
    requires nestable_parsers<OC, C1, C2> &&
    //// can we construct on istream??
    requires(typename C1::value_type val) {
      std::istringstream{ val };
    }
  ref<parser<OC>> for_each(const ref<parser<C1>>& parser_obj, const ref<parser<C2>>& for_each_parser) {
    return parser_obj | [=](const C1& c1) -> OC {
      OC result;
      for (const auto& data : c1) {
        std::istringstream stream(data);
        C2 val = (*for_each_parser)(stream);
        if (stream.fail()) {
          return result;
        }

        result.insert(result.end(), val);
      }
      return result;
    };
  }
  template <typename C, typename T>
  concept can_store =
    is_container_type<C> &&
    (std::same_as<typename C::value_type, T> || std::convertible_to<T, typename C::value_type>);

  template <typename T>
  concept parsable =
    requires(const T& val) { std::istream{ val }; } ||
    requires(const T& val) { std::istringstream{ val }; };

  template <typename T>
  concept iter_parsable =
    is_container_type<T> &&
    parsable<typename T::value_type> &&
    requires(const T& val) {
      std::begin(val);
      std::end(val);
    };

  template <typename C, typename T1, typename T2>
  concept chainable = can_store<C, T2> && (parsable<T1> || iter_parsable<T1>);

  template <typename C, typename T1, typename T2>
  concept not_chainable = !chainable<C, T1, T2>;

  template <typename T1, typename T2>
  using parser_chain_pair = std::tuple<ref<parser<T1>>, ref<parser<T2>>>;

  template <typename C, typename T1, typename T2>
    requires chainable<C, T1, T2>
  struct parse_into : parser<C> {
    const parser_chain_pair<T1, T2> parsers;

    parse_into(const ref<parser<T1>>& parser1, const ref<parser<T2>>& parser2)
        : parsers({ parser1, parser2 }) {}

    parse_into(const parser_chain_pair<T1, T2>& parsers)
        : parsers(parsers) {}
    C operator()(std::istream& stream) const override {
      auto& [parsers1, parser2] = parsers;

      T1 val = (*parsers1)(stream);
      if (stream.fail()) {
        return C{};
      }

      C result{};
      T2 val2{};
      if constexpr (parsable<T1>) {
        std::istringstream stream2(val);
        val2 = (*parser2)(stream2);

        if (stream.fail()) {
          return C{};
        }
      } else if constexpr (iter_parsable<T1>) {
        for (const auto& data : val) {
          std::istringstream stream2(data);
          val2 = (*parser2)(stream2);

          if (stream2.fail()) {
            return C{};
          }
          result.insert(result.end(), val2);
        }
      } else {
        static_assert(false, "Invalid type");
      }

      return result;
    }
  };
  template <typename C, typename T>
    requires parsable<T> && std::same_as<T, typename C::value_type>
  struct parse_and_collect : parser<C> {
    const ref<parser<T>> parser1;
    const ref<parser<C>> parser2;

    parse_and_collect(const ref<parser<T>>& parser1, const ref<parser<C>>& parser2)
        : parser1(parser1), parser2(parser2) {}

    C operator()(std::istream& stream) const override {
      T val = (*parser1)(stream);
      if (stream.fail()) {
        return C{};
      }

      if (val == T{}) {
        stream.setstate(std::ios_base::failbit);
        return C{};
      }

      std::istringstream str2(val);
      C result = (*parser2)(str2);
      if (str2.fail()) {
        return C{};
      }

      return result;
    }
  };
  template <typename C, typename T1, typename T2>
    requires chainable<C, T1, T2>
  ref<parser<C>> into(const ref<parser<T1>>& parser_obj, const ref<parser<T2>>& into_parser) {
    return make_ref<parse_into<C, T1, T2>>(parser_obj, into_parser);
  }

  template <typename C, typename T1, typename T2>
    requires chainable<C, T1, T2>
  ref<parser<C>> into(const parser_chain_pair<T1, T2>& parsers) {
    return make_ref<parse_into<C, T1, T2>>(parsers);
  }

  template <typename C, typename T>
    requires parsable<T> && std::same_as<T, typename C::value_type>
  ref<parser<C>> collect_into(const ref<parser<T>>& parser_obj, const ref<parser<C>>& into_parser) {
    return make_ref<parse_and_collect<C, T>>(parser_obj, into_parser);
  }
  template <typename T>
  struct maybe_parser : parser<std::optional<T>> {
    ref<parser<T>> parser_obj = nullptr;

    maybe_parser(const ref<parser<T>>& parser_obj)
        : parser_obj(parser_obj) {}

    std::optional<T> operator()(std::istream& stream) const override {
      std::streampos before_parse = stream.tellg();
      try {
        if (stream.eof()) {
          return std::nullopt;
        }
        T val = (*parser_obj)(stream);
        if (!stream.fail()) {
          return val;
        }

        throw parsing_error();
      } catch (...) {
        stream.clear();
        stream.seekg(before_parse);
        update_stream(stream);
        return std::nullopt;
      }
    }
  };
  template <typename T>
  ref<parser<std::optional<T>>> maybe(ref<parser<T>> parser_obj) {
    return make_ref<maybe_parser<T>>(parser_obj);
  }

  // template <typename T>
  // struct BodyParser : parser<T> {
  //   char list_start, list_end;
  //   ref<parser<T>> parser = nullptr;

  //   BodyParser(char list_start, char list_end, const ref<parser<T>>& parser)
  //       : list_start(list_start), list_end(list_end), parser(parser) {}

  //   T operator()(std::istream& stream) const override {
  //     try {
  //       while (!stream.eof() && std::isspace(stream.peek())) {
  //         stream.get();
  //       }

  //       char c = stream.peek();  // '{'
  //       if (c != list_start) {
  //         stream.setstate(std::ios::failbit);
  //         return T{};
  //       }
  //       stream.ignore();  // consume '{'

  //       T val = (*parser)(stream);
  //       if (stream.fail()) {
  //         return T{};
  //       }

  //       while (!stream.eof() && std::isspace(stream.peek())) {
  //         stream.get();
  //       }

  //       c = stream.peek();  // '}'
  //       if (c != '}') {
  //         stream.setstate(std::ios::failbit);
  //         return T{};
  //       }
  //       stream.ignore();  // consume '}'

  //       return val;
  //     } catch (...) {
  //       return T{};
  //     }
  //   }
  // };

}  // namespace other

#endif  // !OTHER_ENGINE_PARSER_COMBINATORS_HPP
