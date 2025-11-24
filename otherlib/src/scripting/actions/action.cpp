/**
 * \file scripting/actions/action.cpp
 **/
#include "scripting/actions/action.hpp"

namespace other {

  void action::set_callback(scope<callback> cb) {
    callback_fn = std::move(cb);
  }

  value action::execute(const std::span<value> args) {
    if (callback_fn) {
      return callback_fn->call(args);
    } else {
      return value();
    }
  }

}  // namespace other