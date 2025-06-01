/**
 * \file command/command_parser.cpp
 **/
#include "command/command_parser.hpp"

#include <fstream>

#include "core/logger.hpp"

#include "serialization/parser_combinators.hpp"

namespace other {

  command_parser::command_parser() {
    auto command_name = skip_spaces() >> build_command_name_parser();
    auto args = skip_spaces() >> many<std::vector<std::string>>(skip_spaces() >> match_any_word());
    parser_obj = parse_multiple(command_name, args);
  }

  std::vector<raw_command> command_parser::parse_file(const filepath& file_path) const {
    OTHER_ASSERT(parser_obj != nullptr, "Parser is null");

    std::ifstream file{ file_path };
    if (!file.is_open()) {
      CORE_LOG_ERROR("Failed to open command file : {}", file_path.string());
      return {};
    }

    std::vector<raw_command> commands;
    std::string line;
    while (std::getline(file, line)) {
      commands.push_back(parse(line));
    }

    return commands;
  }

  raw_command command_parser::parse(const std::string_view str) const {
    std::istringstream stream{ std::string{ str } };

    std::tuple<std::string, std::vector<std::string>> result;
    try {
      result = (*parser_obj)(stream);
    } catch (const parsing_error& e) {
      return raw_command{
        .name = "invalid",
      };
    }

    auto [name, args] = result;
    return raw_command{
      .name = name,
      .args = args,
    };
  }

  ref<parser<std::string>> command_parser::build_command_name_parser() {
    std::vector<std::string> command_names;
    for (const auto& cmd : kAvailableCommands) {
      command_names.push_back(std::string{ cmd.name });
    }

    return match_any_string_from(command_names);
  }

}  // namespace other