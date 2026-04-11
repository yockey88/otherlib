/**
 * \file scripting/dotnet_bindings.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_DOTNET_BINDINGS_HPP
#define OTHERLIB_SCRIPTING_DOTNET_BINDINGS_HPP

#include "dotnet/host.hpp"

namespace other {

  class driver;

  void set_dotnet_native_driver(driver* drv);
  void bind_otherlib_dotnet_functions(dotnet_host& dn_host);

  namespace bindings {

    void native_event_register(native_string event_name);
    void native_event_trigger(native_string event_name);
    void native_event_trigger_with_string(native_string event_name, native_string data);

    nbool32 native_input_is_key_down(int32_t sdl_scancode);
    nbool32 native_input_is_key_pressed(int32_t sdl_scancode);
    nbool32 native_input_is_mouse_button_down(int32_t button);
    nbool32 native_input_is_mouse_button_clicked(int32_t button);
    void native_input_get_mouse_position(float* out_x, float* out_y);
    void native_input_get_mouse_delta(float* out_x, float* out_y);
    float native_input_get_mouse_wheel();

    float native_time_get_delta_time();
    float native_time_get_elapsed_time();
    int64_t native_time_get_frame_count();

    int32_t native_driver_get_state();
    void native_driver_request_shutdown();
    native_string native_driver_get_project_name();

    native_string native_config_get_string(native_string section, native_string key, native_string default_value);
    int32_t native_config_get_int(native_string section, native_string key, int32_t default_value);
    float native_config_get_float(native_string section, native_string key, float default_value);
    nbool32 native_config_get_bool(native_string section, native_string key, nbool32 default_value);

    nbool32 native_network_is_connected();
    int32_t native_network_get_role();

  }  // namespace bindings
}  // namespace other

#endif  // OTHERLIB_SCRIPTING_DOTNET_BINDINGS_HPP