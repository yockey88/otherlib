/**
 * \file core/command_line.hpp
 **/
#ifndef OTHER_CORE_COMMAND_LINE_HPP
#define OTHER_CORE_COMMAND_LINE_HPP

#include <string>
#include <vector>

#include "core/defines.hpp"

namespace other {

  struct command_line {
    struct diangostic_flags {
      bool help = false;
      bool usage = false;
      bool verbose = false;
    };

    bool valid = false;
    diangostic_flags diagnostics;

    std::string config_file = "";
    opt<filepath> working_directory = std::nullopt;

    opt<integer_t> session_id = std::nullopt;  // Session ID for the server, if applicable
    opt<uint16_t> port = 49222;                // Port to use for server communication, default is 49222

    /// rest of command line arguments without specific options
    std::vector<std::string> args;

    opt<filepath> project_file = std::nullopt;  // Path to the project file to load on startup, if applicable

    static command_line parse(int* argc, char* argv[]);
  };

}  // namespace other

#endif  // OTHER_CORE_COMMAND_LINE_HPP