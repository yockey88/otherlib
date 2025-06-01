/**
 * \file command/command_parser.hpp
 **/
#ifndef OTHER_CORE_COMMAND_COMMAND_PARSER_HPP
#define OTHER_CORE_COMMAND_COMMAND_PARSER_HPP

#include "command/command.hpp"
#include "serialization/parser_combinators.hpp"

namespace other {

  class command_parser {
   public:
    command_parser();

    std::vector<raw_command> parse_file(const filepath& file_path) const;
    raw_command parse(const std::string_view str) const;

   private:
    ref<parser<std::tuple<std::string, std::vector<std::string>>>> parser_obj = nullptr;

    ref<parser<std::string>> build_command_name_parser();
  };

}  // namespace other

#endif  // OTHER_CORE_COMMAND_COMMAND_PARSER_HPP