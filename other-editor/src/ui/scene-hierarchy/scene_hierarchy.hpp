/**
 * \file ui/scene-hierarchy/scene_hierarchy.hpp
 **/
#ifndef OTHERLIB_UI_SCENE_HIERARCHY_HPP
#define OTHERLIB_UI_SCENE_HIERARCHY_HPP

#include <unordered_set>

#include "ui/ui_window.hpp"

#include "editor_context.hpp"

namespace other {

  class driver;

  namespace ui {

    class scene_hierarchy : public ui_window {
     public:
      scene_hierarchy(editor_context& ctx, event_system& events, driver* drvr);
      virtual ~scene_hierarchy() = default;

      void on_render_body() override;

     private:
      editor_context& editor_ctx;
      driver* driver_ptr = nullptr;

      constexpr inline static size_t kBufferSize = 256;

      char filter_buf[kBufferSize] = {};
      std::string filter_lower;

      /// expansion is panel state, not selection state; ids go stale across snapshot
      ///  restores which just collapses those rows
      std::unordered_set<natural_t> expanded_ids;

      /// {dragged, new parent}; applied after the tree walk so the walk never mutates
      ///  the child lists it is iterating
      opt<std::pair<natural_t, natural_t>> pending_reparent;

      natural_t renaming_object_id = 0;
      bool rename_popup_pending = false;
      char rename_buf[kBufferSize] = {};

      void begin_rename(scene_object& object);
      void accept_reparent_drop(natural_t new_parent_id);

      void draw_object_context_menu(scene* active_scene, scene_object& object);
      void draw_hierarchy_context_menu(scene* active_scene);
      void draw_rename_popup(scene* active_scene);

      bool passes_filter(scene* active_scene, scene_object& object) const;
      void draw_object_tree(scene* active_scene, scene_object& object, uint32_t indent_level, bool& row_right_clicked);
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_SCENE_HIERARCHY_HPP
