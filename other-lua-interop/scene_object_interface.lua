local _SceneObjectInterface = {}

function _SceneObjectInterface:new(name, position)
  local obj = {}
  obj.native_id = Other:Scene().CreateSceneObject(name, position)
  obj.Transform = _NativeTransformComponent:new()
  obj.Scripts = _NativeScriptComponent:new()
  function obj:AttachBehavior(behavior_type_name)
    Other:Scene():AttachDotNetBehaviorToObject(obj.native_id, behavior_type_name)
  end

  setmetatable(obj, self)
  self.__index = self

  if name ~= nil
  then
    Other:Scene():SetObjectName(obj.native_id, name)
  end
  return obj
end

function _SceneObjectInterface:new_parented(name, position, parent_id)
  local obj = {}
  obj.native_id = Other:Scene().CreateParentedSceneObject(name, position, parent_id)
  obj.Transform = _NativeTransformComponent:new()
  obj.Scripts = _NativeScriptComponent:new()
  function obj:AttachBehavior(behavior_type_name)
    Other:Scene():AttachDotNetBehaviorToObject(obj.native_id, behavior_type_name)
  end

  setmetatable(obj, self)
  self.__index = self

  if name ~= nil
  then
    Other:Scene():SetObjectName(obj.native_id, name)
  end
  return obj
end

function _SceneObjectInterface:AddComponent(component_name)
  return Other:Scene():AddComponentToObject(self.native_id, component_name)
end
function _SceneObjectInterface:GetComponent(component_name)
  return Other:Scene():GetObjectComponent(self.native_id, component_name)
end
function _SceneObjectInterface:RemoveComponent(component_name)
  return Other:Scene():RemoveComponentFromObject(self.native_id, component_name)
end


function _SceneObjectInterface:GetId()
  return self.native_id
end
function _SceneObjectInterface:Name()
  return Other:Scene():GetObjectName(self.native_id)
end
function _SceneObjectInterface:SetName(name)
  Other:Scene():SetObjectName(self.native_id, name)
end
function _SceneObjectInterface:AddTag(tag)
  Other:Scene():AddObjectTag(self.native_id, tag)
end
function _SceneObjectInterface:RemoveTag(tag)
  Other:Scene():RemoveObjectTag(self.native_id, tag)
end

function _SceneObjectInterface:AttachModel(model_path)
  return Other:Scene():AttachModelToObject(self.native_id, model_path)
end
function _SceneObjectInterface:AttachCamera()
  return Other:Scene():AttachCameraToObject(self.native_id)
end
function _SceneObjectInterface:AttachPointLight(pl)
  return Other:Scene():AttachPointLightToObject(self.native_id, pl)
end
function _SceneObjectInterface:AttachDirectionLight(dl)
  return Other:Scene():AttachDirectionLightToObject(self.native_id, dl)
end

return _SceneObjectInterface