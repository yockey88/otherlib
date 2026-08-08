/**
 * \file scripting/dotnet_bindings/event_bindings.cpp
 **/
#include "scripting/dotnet_bindings/event_bindings.hpp"

#include "core/logger.hpp"
#include "event/event_system.hpp"

#include "scripting/dotnet_bindings/driver_bindings.hpp"

#include "driver/driver.hpp"

namespace other {
  namespace bindings {

    namespace {

      event_system* events() {
        driver* d = detail::get_dotnet_native_driver_unchecked();
        return d != nullptr ? d->get_event_system().get() : nullptr;
      }

    }  // namespace

    void native_event_register(native_string event_name) {
      if (event_system* ev = events(); ev != nullptr) {
        ev->register_event(static_cast<std::string>(event_name));
      }
    }

    void native_event_trigger(native_string event_name) {
      if (event_system* ev = events(); ev != nullptr) {
        ev->trigger_event(static_cast<std::string>(event_name));
      }
    }

    void native_event_trigger_with_string(native_string event_name, native_string data) {
      if (event_system* ev = events(); ev != nullptr) {
        ev->trigger_event(static_cast<std::string>(event_name), static_cast<std::string>(data));
      }
    }

  }  // namespace bindings
}  // namespace other
