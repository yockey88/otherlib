/**
 * \file renderer/ui/ui_node.hpp
 **/
#ifndef OTHER_RENDERER_UI_UI_NODE_HPP
#define OTHER_RENDERER_UI_UI_NODE_HPP

#include <deque>
#include <stack>
#include <string_view>

#include <glm/glm.hpp>

#include "core/defines.hpp"
#include "core/fnv.hpp"
#include "core/interfaces.hpp"
#include "core/scope.hpp"
#include "event/event_system.hpp"

#include "ui/ui_helpers.hpp"

namespace other {

  class ui_window;

  class OTHER_CLASS ui_node : public ref_counted {
    OTHER_ENVIRONMENT_INTERFACE("Renderer", "UINode", ui_window*);

   public:
    ui_node(ui_window* parent, const std::string_view node_title, const glm::vec2& size_arg = { 0.f, 0.f }, int32_t child_flags = 0, int32_t window_flags = 0)
        : id(FNV(node_title)), node_title(node_title), size(size_arg), flags(child_flags), window_flags(window_flags), containing_window(parent) {}
    virtual ~ui_node() = default;

    void refresh();
    void render();

    natural_t add_child_node(ref<ui_node> node);
    natural_t add_node_to(ref<ui_node> node, const std::string_view remaining_search_pattern = "");

    void set_size(const glm::vec2& new_size) { size = new_size; }
    glm::vec2 get_size() const { return size; }

    int32_t get_flags() const { return flags; }
    int32_t get_window_flags() const { return window_flags; }

    natural_t parent = 0;
    std::vector<natural_t> children;

    natural_t id = 0;
    std::string node_title;

    bool visited_this_frame = false;

   protected:
    virtual void on_refresh() {}

    virtual void on_prepare_render() {}
    virtual void on_render_node_header() {}
    virtual void on_render_node_body() {}
    virtual void on_render_node_footer() {}
    virtual void on_render_end() {}

    event_system& events();

    void add_child_flag(uint32_t flag) { flags |= flag; }
    void remove_child_flag(uint32_t flag) { flags &= ~flag; }

    void add_window_flag(uint32_t flag) { window_flags |= flag; }
    void remove_window_flag(uint32_t flag) { window_flags &= ~flag; }

    void trigger_event(const std::string_view name);

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

    ui_window* containing_window = nullptr;

    void refresh(bool current_state);
  };

}  // namespace other

#endif  // OTHER_RENDERER_UI_UI_NODE_HPP