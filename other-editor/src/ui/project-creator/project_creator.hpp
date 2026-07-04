/**
 * \file ui/project-creator/project_creator.hpp
 **/
#ifndef OTHER_EDITOR_UI_PROJECT_CREATOR_PROJECT_CREATOR_HPP
#define OTHER_EDITOR_UI_PROJECT_CREATOR_PROJECT_CREATOR_HPP

#include "ui/ui_window.hpp"

#include "editor_context.hpp"

namespace other {
  namespace ui {

    class project_creator : public ui_window {
     public:
      project_creator(editor_context& ctx, event_system& events);
      virtual ~project_creator() = default;

     protected:
      void on_render_header() override;
      void on_render_body() override;

     private:
      editor_context& editor_ctx;

      std::string new_project_name;
      filepath project_directory;
      filepath project_file;

      constexpr static inline size_t kProjectNameBufferSize = 256;
      std::array<char, kProjectNameBufferSize> project_name_buf;
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHER_EDITOR_UI_PROJECT_CREATOR_PROJECT_CREATOR_HPP