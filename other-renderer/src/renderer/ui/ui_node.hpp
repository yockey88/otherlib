/**
 * \file renderer/ui/ui_node.hpp
 **/
#ifndef OTHER_RENDERER_UI_UI_NODE_HPP
#define OTHER_RENDERER_UI_UI_NODE_HPP

#include <deque>
#include <string_view>

#include "core/defines.hpp"
#include "core/fnv.hpp"
#include "core/scope.hpp"

#include "glm/fwd.hpp"

namespace other {

  class OTHER_CLASS ui_node {
   public:
    ui_node(const std::string_view node_title, const glm::vec2& size_arg = { 0.f, 0.f }, int32_t child_flags = 0, int32_t window_flags = 0)
        : id(FNV(node_title)), node_title(node_title), size(size_arg), flags(child_flags), window_flags(window_flags) {}
    virtual ~ui_node() = default;

    void render();

    void add_child_node(scope<ui_node>& node);
    void add_node_to(scope<ui_node>& node, const std::string_view remaining_search_pattern = "");

    natural_t parent = 0;
    std::vector<ui_node*> children;

    natural_t id = 0;
    std::string_view node_title;

    bool visited_this_frame = false;

   protected:
    virtual void on_refresh() {}
    virtual void on_render_start() {}
    virtual void render_node() {}
    virtual void on_render_end() {}

    void set_size(const glm::vec2& new_size) { size = new_size; }
    glm::vec2 get_size() const { return size; }

    int32_t get_flags() const { return flags; }
    int32_t get_window_flags() const { return window_flags; }

   private:
    struct {
      bool open = true;
      bool just_closed = false;
      bool just_opened = false;
      bool is_focused = false;
    } state;

    glm::vec2 size{ 0.0f, 0.0f };
    int32_t flags = 0;
    int32_t window_flags = 0;

    void refresh(bool current_state);
  };

}  // namespace other

#endif  // OTHER_RENDERER_UI_UI_NODE_HPP