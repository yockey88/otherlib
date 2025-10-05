/**
 * \file server-dev/server-ui/project-window.hpp
 **/
#ifndef OTHER_SERVER_SEVRER_UI_PROJECT_WINDOW_HPP
#define OTHER_SERVER_SEVRER_UI_PROJECT_WINDOW_HPP

#include <nlohmann/json.hpp>

#include "renderer/ui/ui_window.hpp"

namespace json = nlohmann;

namespace other {

  class project_window : public ui_window {
   public:
    project_window(json::json& project_cache);
    ~project_window() = default;

   private:
    json::json& project_cache;
  };

}  // namespace other

#endif  // OTHER_SERVER_SEVRER_UI_PROJECT_WINDOW_HPP