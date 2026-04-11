/**
 * \file scripting/dotnet_bindings/driver_bindings.hpp
 **/
#ifndef OTHER_ENVIRONMENT_SCRIPTING_DOTNET_BINDINGS_DRIVER_BINDINGS_HPP
#define OTHER_ENVIRONMENT_SCRIPTING_DOTNET_BINDINGS_DRIVER_BINDINGS_HPP

#include "dotnet/native_string.hpp"

namespace other {

  class driver;

  namespace detail {

    driver* get_dotnet_native_driver_unchecked();
    driver* get_dotnet_native_driver();
    void set_dotnet_native_driver(driver* drv);

  }  // namespace detail
  namespace bindings {

    int32_t native_driver_get_state();
    void native_driver_request_shutdown();
    native_string native_driver_get_project_name();

  }  // namespace bindings
}  // namespace other

#endif  // OTHER_ENVIRONMENT_SCRIPTING_DOTNET_BINDINGS_DRIVER_BINDINGS_HPP