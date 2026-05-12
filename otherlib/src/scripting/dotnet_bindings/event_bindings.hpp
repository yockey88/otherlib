/**
 * \file scripting/dotnet_bindings/event_bindings.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_DOTNET_BINDINGS_EVENT_BINDINGS_HPP
#define OTHERLIB_SCRIPTING_DOTNET_BINDINGS_EVENT_BINDINGS_HPP

#include "dotnet/native_string.hpp"

namespace other {
  namespace bindings {

    void native_event_register(native_string event_name);
    void native_event_trigger(native_string event_name);
    void native_event_trigger_with_string(native_string event_name, native_string data);

  }  // namespace bindings
}  // namespace other

#endif  // OTHERLIB_SCRIPTING_DOTNET_BINDINGS_EVENT_BINDINGS_HPP