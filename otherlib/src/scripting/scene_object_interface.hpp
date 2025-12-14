/**
 * \file scripting/scene_object_interface.hpp
 **/
#ifndef OTHERLIB_SCRIPTING_SCENE_OBJECT_INTERFACE_HPP
#define OTHERLIB_SCRIPTING_SCENE_OBJECT_INTERFACE_HPP

#include "lua/lua_host.hpp"

namespace other {

  class driver;

  class scene_object_interface {
   public:
    static void bind_scene_object_interface_lua_functions(lua_host& lua_host, driver* host_driver);

   private:
  };

}  // namespace other

#endif  // OTHERLIB_SCRIPTING_SCENE_OBJECT_INTERFACE_HPP