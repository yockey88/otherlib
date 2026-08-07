/**
 * \file ui/type_database.cpp
 **/
#include "ui/type_database.hpp"

#include "core/profiler.hpp"
#include "serialization/reflection.hpp"

namespace other {
  namespace ui {

    struct type_list : public ui_node {
      type_list(ui_window* parent)
          : ui_node(parent, "Registered Types") {}
      virtual ~type_list() = default;

      virtual void on_render_node_body() override {
        PROFILE_SECTION("type_list::on_render_node_body");
        auto* type_db = subsystem<other::type_database>::get();
        OTHER_ASSERT(type_db != nullptr, "Type database subsystem is not initialized in ui::type_database::type_list::on_render_node_body");

        const auto& types = type_db->get_type_data();
        for (const auto& [type_hash, refl_data] : types) {
          ImGui::Text("Type: %s (Hash: %llu)", refl_data.type_name.c_str(), type_hash);
        }
      }
    };

    type_database::type_database(event_system& event)
        : ui_window(&event, "Type Database") {
      auto type_list_node = make_ref<type_list>(this);
      add_node(type_list_node);
    }

  }  // namespace ui
}  // namespace other