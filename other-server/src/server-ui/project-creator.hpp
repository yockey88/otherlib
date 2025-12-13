/**
 * \file server-ui/project-creator.hpp
 **/
#ifndef OTHER_SERVER_SERVER_UI_PROJECT_CREATOR_HPP
#define OTHER_SERVER_SERVER_UI_PROJECT_CREATOR_HPP

#include "renderer/ui/ui_window.hpp"

namespace other {

  class project_creator : public ui_window {
   public:
    project_creator(event_system& events);

    void create_project();

    struct project_context {
      filepath project_path;
      filepath working_directory;
      std::string project_name;

      constexpr static inline size_t kMaxProjectNameLength = 128;
      constexpr static inline size_t kMaxProjectPathLength = 512;
      std::array<char, kMaxProjectNameLength> project_name_buffer;
      std::array<char, kMaxProjectPathLength> project_path_buffer;
      std::array<char, kMaxProjectPathLength> working_directory_buffer;
    };
    project_context context;
    opt<std::string> error_message;

   private:
  };

}  // namespace other

#endif  // OTHER_SERVER_SERVER_UI_PROJECT_CREATOR_HPP