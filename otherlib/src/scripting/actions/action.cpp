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

}  // namespace other