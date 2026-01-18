/**
 * \file scripting/actions/builtin_actions.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_ACTIONS_BUILTIN_ACTIONS_HPP
#define OTHERLIB_SCRIPTING_ACTIONS_BUILTIN_ACTIONS_HPP

#include "scripting/action.hpp"

namespace other {

  // struct print_action : public action {
  //   print_action(const std::string_view name = "Print", const std::string_view description = "Prints the input value to the console.")
  //       : action(name, description) {}
  //   virtual ~print_action() = default;

  //   value execute(const value& v) override {
  //     CORE_LOG_INFO("Print Action Output: {}", v.to_string());
  //     return v;
  //   }
  // };

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_ACTIONS_BUILTIN_ACTIONS_HPP