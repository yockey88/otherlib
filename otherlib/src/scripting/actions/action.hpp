/**
 * \file scripting/actions/action.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_ACTIONS_ACTION_HPP
#define OTHERLIB_SCRIPTING_ACTIONS_ACTION_HPP

#include <string>

#include "core/scope.hpp"
#include "core/value.hpp"

#include "scripting/actions/callback.hpp"

namespace other {

  struct action {
    std::string name;
    std::string description;

    action(const std::string_view name, const std::string_view description)
        : name(name), description(description) {}
    action(const std::string_view name, const std::string_view description, scope<callback> cb)
        : name(name), description(description), callback_fn(std::move(cb)) {}
    virtual ~action() = default;

    void set_callback(scope<callback> cb);
    value execute(const std::span<value> args);

   private:
    scope<callback> callback_fn;
  };

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_ACTIONS_ACTION_HPP