/**
 * \file renderer/ui/ui_window.hpp
 **/
#ifndef OTHER_RENDERER_UI_UI_WINDOW_HPP
#define OTHER_RENDERER_UI_UI_WINDOW_HPP

#include <deque>

#include "core/defines.hpp"
#include "core/fnv.hpp"
#include "core/scope.hpp"

#include "renderer/ui/ui_node.hpp"

namespace other {

  class OTHER_CLASS ui_window {
   public:
    struct window_root : public ui_node {
      window_root()
          : ui_node("root") {}
      virtual ~window_root() = default;
    };

   public:
    ui_window(const std::string_view title, bool open = true, int32_t flags = 0);
    virtual ~ui_window() = default;

    void render();

    void add_node(scope<ui_node> node);
    void add_node_to(scope<ui_node>& node, const std::string_view remaining_search_pattern = "");
    scope<ui_node>& get_node(natural_t node_id);

    natural_t id = 0;
    std::string title;

   protected:
    virtual void on_refresh() {}
    virtual void on_render_start() {}
    virtual void on_render_end() {}

    bool is_window_open() const { return state.open; }
    bool was_window_just_closed() const { return state.just_closed; }
    bool was_window_just_opened() const { return state.just_opened; }
    bool is_window_focused() const { return state.is_focused; }

    void add_window_flag(uint32_t flag) { window_flags |= flag; }
    void remove_window_flag(uint32_t flag) { window_flags &= ~flag; }

   private:
    struct {
      bool open = true;
      bool just_closed = false;
      bool just_opened = false;
      bool is_focused = false;
    } state;
    uint32_t window_flags = 0;
    std::unordered_map<natural_t, scope<ui_node>> node_map;

    void refresh(bool current_state);
  };

}  // namespace other

#endif  // OTHER_RENDERER_UI_UI_WINDOW_HPP