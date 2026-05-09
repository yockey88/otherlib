/**
 * \file scripting/dotnet_bindings/driver_bindings.hpp
 **/
#ifndef OTHER_ENVIRONMENT_SCRIPTING_DOTNET_BINDINGS_DRIVER_BINDINGS_HPP
#define OTHER_ENVIRONMENT_SCRIPTING_DOTNET_BINDINGS_DRIVER_BINDINGS_HPP

#include "dotnet/native_string.hpp"
#include "dotnet/types.hpp"

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

    float native_time_get_delta_time();
    float native_time_get_elapsed_time();
    int64_t native_time_get_frame_count();

    native_string native_config_get_string(native_string section, native_string key, native_string default_value);
    int32_t native_config_get_int(native_string section, native_string key, int32_t default_value);
    float native_config_get_float(native_string section, native_string key, float default_value);
    nbool32 native_config_get_bool(native_string section, native_string key, nbool32 default_value);

  }  // namespace bindings
}  // namespace other

#endif  // OTHER_ENVIRONMENT_SCRIPTING_DOTNET_BINDINGS_DRIVER_BINDINGS_HPP