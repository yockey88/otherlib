/**
 * \file scripting/environment_interface.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_ENVIRONMENT_INTERFACE_HPP
#define OTHERLIB_SCRIPTING_ENVIRONMENT_INTERFACE_HPP

#include <string>
#include <vector>

#include "scripting/actions/action.hpp"

namespace other {

  struct interface_method {
    std::string name = "UnnamedMethod";
    std::string description;
    bool required = false;

    opt<std::string> script_name;
    opt<std::string> plugin_name;
  };

  struct bound_interface_method {
    std::string method_id;
    action method_action;
  };

  struct environment_interface {
    std::string name;
    std::string description;
    ostd::vector<interface_method> actions;
  };

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_ENVIRONMENT_INTERFACE_HPP