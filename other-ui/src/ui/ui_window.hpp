/**
 * \file renderer/ui/ui_window.hpp
 **/
#ifndef OTHER_RENDERER_UI_UI_WINDOW_HPP
#define OTHER_RENDERER_UI_UI_WINDOW_HPP

#include "core/defines.hpp"
#include "core/interfaces.hpp"
#include "core/scope.hpp"
#include "event/event_system.hpp"

#include "ui/ui_node.hpp"

namespace other {

  class driver;

  class ui_window {
    OTHER_ENVIRONMENT_INTERFACE("Renderer", "UIWindow", event_system*);

   public:
    struct window_root : public ui_node {
      window_root(ui_window* parent)
          : ui_node(parent, "root") {}
      virtual ~window_root() = default;
    };

   public:
    ui_window(event_system* events, const std::string_view title, bool open = true, int32_t flags = 0);
    virtual ~ui_window() = default;

    void set_driver_ptr(driver* drv) { driver_ptr = drv; }

    void initialize();
    void shutdown();

    // true if open
    bool render();

    inline bool just_closed() const { return state.just_closed; }
    inline bool just_opened() const { return state.just_opened; }

    void toggle_open();
    void toggle_close();

    natural_t add_node(ref<ui_node> node);
    natural_t add_node(ref<ui_node> node, const std::string_view parent_search_pattern);
    natural_t add_node_to(ref<ui_node> node, const std::string_view remaining_search_pattern = "");

    ref<ui_node> get_node(natural_t node_id);
    ref<ui_node> get_node_by_name(const std::string_view node_name);
    ref<ui_node> get_node_by_search_pattern(const std::string_view search_pattern);

    template <typename T>
    T& get_node_as(natural_t node_id) {
      auto node = get_node(node_id);
      OTHER_ASSERT(node != nullptr, "UI node with ID {} is null in window {}", node_id, title);

      auto* casted_node = dynamic_cast<T*>(node.raw_ptr());
      OTHER_ASSERT(casted_node != nullptr, "UI node with ID {} is not of requested type in window {}", node_id, title);
      return *casted_node;
    }
    template <typename T>
    T& get_node_as(const std::string_view search_pattern) {
      auto node = get_node_by_search_pattern(search_pattern);
      OTHER_ASSERT(node != nullptr, "UI node with search pattern '{}' is null in window {}", search_pattern, title);

      auto* casted_node = dynamic_cast<T*>(node.raw_ptr());
      OTHER_ASSERT(casted_node != nullptr, "UI node with search pattern '{}' is not of requested type in window {}", search_pattern, title);
      return *casted_node;
    }

    event_system& get_event_system() {
      OTHER_ASSERT(events != nullptr, "Event system pointer is null in UI window {}", title);
      return *events;
    }
    driver& get_driver() {
      OTHER_ASSERT(driver_ptr != nullptr, "Driver pointer is null in UI window {}", title);
      return *driver_ptr;
    }

    natural_t id = 0;
    std::string title;

   protected:
    virtual void on_initialize() {}
    virtual void on_shutdown() {}

    virtual void on_prepare_render() {}
    virtual void on_refresh() {}
    virtual void on_render_header() {}
    virtual void on_render_body() {}
    virtual void on_pre_render_nodes() {}
    virtual void on_post_render_nodes() {}
    virtual void on_render_footer() {}
    virtual void on_render_end() {}

    integer_t script_object_id = -1;

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

    std::unordered_map<natural_t, ref<ui_node>> node_map;

    driver* driver_ptr = nullptr;
    event_system* events;

    void refresh(bool current_state);
  };

  inline auto ui_window_args(event_system* jobs) {
    return [jobs]() {
      return std::tuple<event_system*>{ jobs };
    };
  }

}  // namespace other

#endif  // OTHER_RENDERER_UI_UI_WINDOW_HPP