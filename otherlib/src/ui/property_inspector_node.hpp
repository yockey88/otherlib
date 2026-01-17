/**
 * \file ui/property_inspector_node.hpp
 **/
#ifndef OTHERLIB_UI_PROPERTY_INSPECTOR_NODE_HPP
#define OTHERLIB_UI_PROPERTY_INSPECTOR_NODE_HPP

#include "renderer/ui/ui_node.hpp"

namespace other {

  class driver;

  namespace ui {

    class property_inspector_node : public ui_node {
     public:
      property_inspector_node(ui_window* window, driver* drvr);
      virtual ~property_inspector_node() = default;

     private:
      driver* driver_ptr = nullptr;

      bool multi_selection_enabled = false;
      std::deque<natural_t> selected_object_ids;

      void handle_object_selection(natural_t object_id);

      void on_render_node_body() override;
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_PROPERTY_INSPECTOR_NODE_HPP