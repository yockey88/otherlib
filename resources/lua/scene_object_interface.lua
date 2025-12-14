local _SceneObjectInterface = {}
function _SceneObjectInterface:new(o)
  local obj = {}
  obj.Transform = __native_transform_component:new()
  obj.Scripts = __native_script_component:new()
  -- obj.Renderable = __native_render_component:new()
  -- __other_native.__scene_object_interface.initialize_scene_object(obj, o)

  setmetatable(obj, self)
  self.__index = self
  return obj
end

return _SceneObjectInterface