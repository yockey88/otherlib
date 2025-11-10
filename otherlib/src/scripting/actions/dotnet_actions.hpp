/**
 * \file scripting/actions/dotnet_actions.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_ACTIONS_DOTNET_ACTIONS_HPP
#define OTHERLIB_SCRIPTING_ACTIONS_DOTNET_ACTIONS_HPP

#include "scripting/action.hpp"

namespace other {

  class dotnet_object;

  struct dotnet_action : public action {
    dotnet_action(dotnet_object* dn_object, const std::string_view fn_name, const std::string_view name, const std::string_view description)
        : action(name, description), dn_object(dn_object), function_name(fn_name) {}
    virtual ~dotnet_action() = default;

    value execute(const value& v) override {
      return value{};
    }

   private:
    dotnet_object* dn_object = nullptr;
    std::string function_name;
  };

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_ACTIONS_DOTNET_ACTIONS_HPP