/**
 * \file scripting/actions/callback.cpp
 **/
#include "scripting/actions/callback.hpp"

namespace other {

  value callback::call(const std::span<value> args) {
    try {
      return call_impl(args);
    } catch (const callback_error& e) {
      CORE_LOG_ERROR("Callback error: {}", e.what());
      return value();
    } catch (const std::exception& e) {
      CORE_LOG_ERROR("Unexpected error during callback execution: {}", e.what());
      return value();
    } catch (...) {
      CORE_LOG_ERROR("Unknown error during callback execution.");
      return value();
    }
  }

}  // namespace other