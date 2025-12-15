local _SceneObjectInterface = {}

function _SceneObjectInterface:new(name)
  local obj = {}
  obj.native_id = _Meta:Scene().CreateSceneObject()
  obj.Transform = __native_transform_component:new()
  obj.Scripts = __native_script_component:new()

  setmetatable(obj, self)
  self.__index = self

  if name ~= nil
  then
    _Meta:Scene():SetObjectName(obj.native_id, name)
  end
  return obj
end

function _SceneObjectInterface:AddComponent(component_name)
  return _Meta:Scene():AddComponentToObject(self.native_id, component_name)
end
function _SceneObjectInterface:GetId()
  return self.native_id
end
function _SceneObjectInterface:Name()
  return _Meta:Scene():GetObjectName(self.native_id)
end
function _SceneObjectInterface:SetName(name)
  _Meta:Scene():SetObjectName(self.native_id, name)
end


return _SceneObjectInterface