/**
 * \file scripting/action.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_ACTION_HPP
#define OTHERLIB_SCRIPTING_ACTION_HPP

#include <string>

#include "core/value.hpp"

namespace other {

  struct action {
    std::string name;
    std::string description;

    action(const std::string_view name, const std::string_view description)
        : name(name), description(description) {}
    virtual ~action() = default;

    virtual value execute(const value& value) = 0;
  };

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_ACTION_HPP