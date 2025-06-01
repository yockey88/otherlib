/**
 * \file core/command_line.hpp
 **/
#ifndef OTHER_CORE_COMMAND_LINE_HPP
#define OTHER_CORE_COMMAND_LINE_HPP

#include <string>
#include <vector>

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

    /// rest of command line arguments without specific options
    std::vector<std::string> args;

    static command_line parse(int* argc, char* argv[]);
  };

}  // namespace other

#endif  // OTHER_CORE_COMMAND_LINE_HPP