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

    action() = default;
    action(const std::string_view name, const std::string_view description)
        : name(name), description(description) {}
    action(const std::string_view name, const std::string_view description, ref<callback> cb)
        : name(name), description(description), callback_fn(std::move(cb)) {}
    virtual ~action() = default;

    void set_callback(ref<callback> cb);
    bool has_callback() const;
    value execute(const std::span<value> args);

   private:
    ref<callback> callback_fn;
  };

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_ACTIONS_ACTION_HPP