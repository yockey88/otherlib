/**
 * \file scripting/environment_interface.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_ENVIRONMENT_INTERFACE_HPP
#define OTHERLIB_SCRIPTING_ENVIRONMENT_INTERFACE_HPP

#include <string>
#include <vector>

#include "scripting/actions/action.hpp"

namespace other {

  struct environment_interface {
    struct method {
      // both nullopt => native
      opt<std::string> dotnet_name;
      opt<std::string> lua_name;

      action act;
    };

    std::string name;
    std::vector<action> actions;
  };

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_ENVIRONMENT_INTERFACE_HPP