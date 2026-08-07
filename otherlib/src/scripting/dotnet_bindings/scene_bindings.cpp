/**
 * \file scripting/dotnet_bindings/scene_bindings.cpp
 **/
#include "scripting/dotnet_bindings/scene_bindings.hpp"

#include "core/logger.hpp"
#include "core/profiler.hpp"

#include "script/scripting_environment.hpp"

#include "scene/scene.hpp"

#include "driver/driver.hpp"
#include "driver/systems/scene_system.hpp"
#include "scripting/dotnet_bindings/driver_bindings.hpp"

namespace other {
  namespace detail {

    scene* get_active_scene_checked() {
      OTHER_ASSERT(detail::get_dotnet_native_driver() != nullptr, "Driver pointer is null.");
      auto* env = subsystem<scripting_environment>::get();
      OTHER_ASSERT(env != nullptr, "Scripting environment is not initialized.");
      auto* active_scene = detail::get_dotnet_native_driver()->get_kernel().get_core_system<scene_system>().get_active_scene();
      OTHER_ASSERT(active_scene != nullptr, "Active scene is null.");
      return active_scene;
    }

  }  // namespace detail
  namespace bindings {

    natural_t native_scene_create_object(native_string name, float x, float y, float z) {
      PROFILE_SECTION("native_scene_create_object");
      scene* active_scene = detail::get_active_scene_checked();
      if (active_scene == nullptr) {
        CORE_LOG_ERROR("No active scene to create object in.");
        return 0;
      }
      std::string name_str = name;
      scene_object& obj = active_scene->create_object(name_str, glm::vec3(x, y, z));
      return obj.id;
    }

    void native_scene_destroy_object(natural_t id) {
      PROFILE_SECTION("native_scene_destroy_object");
      scene* active_scene = detail::get_active_scene_checked();
      if (active_scene == nullptr) {
        return;
      }
      active_scene->destroy_object(id);
    }

    nbool32 native_scene_has_object(natural_t id) {
      scene* active_scene = detail::get_active_scene_checked();
      if (active_scene == nullptr) {
        return false;
      }
      return active_scene->has_object(id);
    }

    native_string native_scene_get_object_name(natural_t id) {
      scene* active_scene = detail::get_active_scene_checked();
      if (active_scene == nullptr) {
        return native_string::new_str("");
      }
      const scene_object& obj = active_scene->get_object(id);
      return native_string::new_str(obj.name);
    }

    void native_scene_set_object_name(natural_t id, native_string name) {
      scene* active_scene = detail::get_active_scene_checked();
      if (active_scene == nullptr) {
        return;
      }
      scene_object& obj = active_scene->get_object(id);
      obj.name = (std::string)name;
    }

    void native_scene_get_object_ids(natural_t* out_ids, int32_t* out_count, int32_t max_count) {
      PROFILE_SECTION("native_scene_get_object_ids");
      scene* active_scene = detail::get_active_scene_checked();
      if (active_scene == nullptr) {
        *out_count = 0;
        return;
      }
      auto ids = active_scene->get_all_object_ids();
      int32_t count = std::min((int32_t)ids.size(), max_count);
      for (int32_t i = 0; i < count; ++i) {
        out_ids[i] = ids[i];
      }
      *out_count = count;
    }

    natural_t native_scene_get_object_count() {
      scene* active_scene = detail::get_active_scene_checked();
      if (active_scene == nullptr) {
        return 0;
      }
      return active_scene->get_object_count();
    }

    natural_t native_scene_find_object_by_name(native_string name) {
      PROFILE_SECTION("native_scene_find_object_by_name");
      scene* active_scene = detail::get_active_scene_checked();
      if (active_scene == nullptr) {
        return 0;
      }
      std::string name_str = name;
      scene_object* obj = active_scene->find_object(name_str);
      if (obj == nullptr) {
        return 0;
      }
      return obj->id;
    }

    natural_t native_scene_get_parent_id(natural_t id) {
      scene* active_scene = detail::get_active_scene_checked();
      if (active_scene == nullptr) {
        return 0;
      }
      const scene_object* parent = active_scene->get_parent(id);
      return parent != nullptr ? parent->id : 0;
    }

    void native_scene_get_children_ids(natural_t id, natural_t* out_ids, int32_t* out_count, int32_t max_count) {
      scene* active_scene = detail::get_active_scene_checked();
      if (active_scene == nullptr) {
        *out_count = 0;
        return;
      }
      auto ids = active_scene->get_children_ids(id);
      int32_t count = std::min((int32_t)ids.size(), max_count);
      for (int32_t i = 0; i < count; ++i) {
        out_ids[i] = ids[i];
      }
      *out_count = count;
    }

    nbool32 native_scene_object_has_tag(natural_t id, native_string tag) {
      scene* active_scene = detail::get_active_scene_checked();
      if (active_scene == nullptr) {
        return false;
      }
      return active_scene->object_has_tag(id, (std::string)tag);
    }

    void native_scene_add_object_tag(natural_t id, native_string tag) {
      scene* active_scene = detail::get_active_scene_checked();
      if (active_scene == nullptr) {
        return;
      }
      active_scene->add_object_tag(id, (std::string)tag);
    }

    void native_scene_remove_object_tag(natural_t id, native_string tag) {
      scene* active_scene = detail::get_active_scene_checked();
      if (active_scene == nullptr) {
        return;
      }
      active_scene->remove_object_tag(id, (std::string)tag);
    }

    nbool32 native_scene_get_object_visible(natural_t id) {
      scene* active_scene = detail::get_active_scene_checked();
      if (active_scene == nullptr) {
        return false;
      }
      return active_scene->get_object(id).visible;
    }

    void native_scene_set_object_visible(natural_t id, nbool32 visible) {
      scene* active_scene = detail::get_active_scene_checked();
      if (active_scene == nullptr) {
        return;
      }
      active_scene->get_object(id).visible = visible;
    }

  }  // namespace bindings
}  // namespace other