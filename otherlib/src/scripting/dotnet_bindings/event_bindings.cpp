/**
 * \file scripting/dotnet_bindings/event_bindings.cpp
 **/
#include "scripting/dotnet_bindings/event_bindings.hpp"

#include "core/logger.hpp"

namespace other {
  namespace bindings {

    void native_event_register(native_string event_name) {
      /// \todo access event system through driver or global accessor
      CORE_LOG_WARN("native_event_register not yet connected to event system");
    }

    void native_event_trigger(native_string event_name) {
      /// \todo access event system through driver or global accessor
      CORE_LOG_WARN("native_event_trigger not yet connected to event system");
    }

    void native_event_trigger_with_string(native_string event_name, native_string data) {
      /// \todo access event system through driver or global accessor
      CORE_LOG_WARN("native_event_trigger_with_string not yet connected to event system");
    }

  }  // namespace bindings
}  // namespace other