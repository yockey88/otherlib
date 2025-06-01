/**
 * \file renderer/window_manager.hpp
 **/
#ifndef OTHERLIB_RENDERER_WINDOW_MANAGER_HPP
#define OTHERLIB_RENDERER_WINDOW_MANAGER_HPP

#include <map>

#include <SDL3/SDL.h>

#include "SDL3/SDL_video.h"

namespace other {

  class window_manager {
   public:
    window_manager() = default;
    ~window_manager() = default;

    SDL_Window* create_window(const char* title, int width, int height, uint32_t flags);
    void destroy_window(SDL_Window* window);

    SDL_Window* get_window(SDL_WindowID window_id);
    SDL_Window* get_main_window() const;

    std::map<SDL_WindowID, SDL_Window*>& get_all_windows() {
      return windows;
    }

   private:
    /// first window created is the main window
    SDL_Window* main_window = nullptr;
    std::map<SDL_WindowID, SDL_Window*> windows;
  };

}  // namespace other

#endif  // OTHERLIB_RENDERER_WINDOW_MANAGER_HPP