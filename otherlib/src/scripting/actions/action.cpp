/**
 * \file scripting/actions/action.cpp
 **/
#include "scripting/actions/action.hpp"

namespace other {

  void action::set_callback(ref<callback> cb) {
    callback_fn = cb;
  }

  bool action::has_callback() const {
    return callback_fn != nullptr;
  }

  value action::execute(const std::span<value> args) {
    if (callback_fn) {
      return callback_fn->call(args);
    } else {
      return value();
    }
  }

}  // namespace other