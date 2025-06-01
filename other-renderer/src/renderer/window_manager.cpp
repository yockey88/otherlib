/**
 * \file renderer/window_manager.cpp
 **/
#include "renderer/window_manager.hpp"

#include "core/logger.hpp"

namespace other {

  SDL_Window* window_manager::create_window(const char* title, int width, int height, uint32_t flags) {
    SDL_Window* window = SDL_CreateWindow(title, width, height, flags);
    if (window == nullptr) {
      CORE_LOG_ERROR("Failed to create window: {}", SDL_GetError());
      return nullptr;
    }

    SDL_WindowID window_id = SDL_GetWindowID(window);
    windows[window_id] = window;

    if (main_window == nullptr) {
      main_window = window;
    }

    return window;
  }

  void window_manager::destroy_window(SDL_Window* window) {
    if (window == nullptr) {
      CORE_LOG_ERROR("Cannot destroy a null window.");
      return;
    }

    SDL_WindowID window_id = SDL_GetWindowID(window);
    auto itr = windows.find(window_id);
    if (itr != windows.end()) {
      SDL_DestroyWindow(itr->second);
      windows.erase(itr);
      if (main_window == window) {
        main_window = nullptr;  // Reset main window if it was destroyed
      }
    } else {
      CORE_LOG_ERROR("Window with ID {} not found.", window_id);
    }
  }

  SDL_Window* window_manager::get_window(SDL_WindowID window_id) {
    auto itr = windows.find(window_id);
    if (itr != windows.end()) {
      return itr->second;
    }
    CORE_LOG_ERROR("Window with ID {} not found.", window_id);
    return nullptr;
  }

  SDL_Window* window_manager::get_main_window() const {
    if (main_window == nullptr) {
      CORE_LOG_ERROR("Main window is not set.");
      return nullptr;
    }
    return main_window;
  }

}  // namespace other