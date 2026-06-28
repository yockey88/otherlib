/**
 * \file ui/type-bindings/vm_ui.hpp
 **/
#ifndef OTHERLIB_UI_TYPE_BINDINGS_VM_UI_HPP
#define OTHERLIB_UI_TYPE_BINDINGS_VM_UI_HPP

#include <imgui/ImReflect.hpp>

#include "ui/ui_node.hpp"
#include "vm/opcode.hpp"
#include "vm/other_device.hpp"


namespace other {
  namespace ui {

    struct device_display : public ui_node {
      other_command_device& device_ref;

      device_display(other_command_device& device, ui_window* parent, const std::string& name)
          : ui_node(parent, std::format("Device Display {}", name)), device_ref(device) {}
      virtual ~device_display() override = default;

      void on_render_node_body() override;
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_TYPE_BINDINGS_VM_UI_HPP