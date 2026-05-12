/**
 * \file scripting/dotnet_bindings/input_bindings.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_DOTNET_BINDINGS_INPUT_BINDINGS_HPP
#define OTHERLIB_SCRIPTING_DOTNET_BINDINGS_INPUT_BINDINGS_HPP

#include <cstdint>

#include "dotnet/types.hpp"

namespace other {
  namespace bindings {

    nbool32 native_input_is_key_down(int32_t sdl_scancode);
    nbool32 native_input_is_key_pressed(int32_t sdl_scancode);
    nbool32 native_input_is_mouse_button_down(int32_t button);
    nbool32 native_input_is_mouse_button_clicked(int32_t button);
    void native_input_get_mouse_position(float* out_x, float* out_y);
    void native_input_get_mouse_delta(float* out_x, float* out_y);
    float native_input_get_mouse_wheel();

  }  // namespace bindings
}  // namespace other

#endif  // OTHERLIB_SCRIPTING_DOTNET_BINDINGS_INPUT_BINDINGS_HPP