/**
 * \file scripting/dotnet_bindings/input_bindings.cpp
 **/
#include "scripting/dotnet_bindings/input_bindings.hpp"

#include <SDL3/SDL.h>

namespace other {
  namespace bindings {

    nbool32 native_input_is_key_down(int32_t sdl_scancode) {
      const bool* keyboard_state = SDL_GetKeyboardState(nullptr);
      if (keyboard_state == nullptr) {
        return false;
      }
      return keyboard_state[sdl_scancode];
    }

    nbool32 native_input_is_key_pressed(int32_t sdl_scancode) {
      /// \note ImGui tracks pressed (transition) state, SDL only gives current state
      ///       we use ImGui's key mapping for pressed detection
      ImGuiKey imgui_key = static_cast<ImGuiKey>(sdl_scancode);
      return ImGui::IsKeyPressed(imgui_key);
    }

    nbool32 native_input_is_mouse_button_down(int32_t button) {
      return ImGui::IsMouseDown(button);
    }

    nbool32 native_input_is_mouse_button_clicked(int32_t button) {
      return ImGui::IsMouseClicked(button);
    }

    void native_input_get_mouse_position(float* out_x, float* out_y) {
      ImVec2 pos = ImGui::GetMousePos();
      *out_x = pos.x;
      *out_y = pos.y;
    }

    void native_input_get_mouse_delta(float* out_x, float* out_y) {
      ImGuiIO& io = ImGui::GetIO();
      *out_x = io.MouseDelta.x;
      *out_y = io.MouseDelta.y;
    }

    float native_input_get_mouse_wheel() {
      ImGuiIO& io = ImGui::GetIO();
      return io.MouseWheel;
    }

  }  // namespace bindings
}  // namespace other