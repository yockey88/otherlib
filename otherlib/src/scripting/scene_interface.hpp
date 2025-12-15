/**
 * \file scripting/scene_interface.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_SCENE_INTERFACE_HPP
#define OTHERLIB_SCRIPTING_SCENE_INTERFACE_HPP

#include "lua/lua_host.hpp"

namespace other {

  class driver;

  struct scene_object;
  class scene;

  class scene_interface {
   public:
    static void add_component(scene* scene, natural_t id, const std::string_view name);

    static std::string get_object_name(scene* scene, natural_t id);
    static void set_object_name(scene* scene, natural_t id, const std::string_view name);

    static void set_scene_clear_color(scene* scene, float r, float g, float b, float a);
    static glm::vec4 get_scene_clear_color(scene* scene);

   private:
  };

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_SCENE_INTERFACE_HPP