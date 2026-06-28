local function _get_active_scene()
  if __other_native.__native_scene == nil
  then
    return nil
  end

  return __other_native.__native_scene
end

local function _get_scene_clear_color(scene_handle)
  return __other_native.__scene_interface.get_scene_clear_color(scene_handle)
end
local function _set_scene_clear_color(scene_handle, r, g, b, a)
  __other_native.__scene_interface.set_scene_clear_color(scene_handle, r, g, b, a)
end
local function _set_scene_clear_color_vec4(scene_handle, color4)
  _set_scene_clear_color(scene_handle, color4.x, color4.y, color4.z, color4.w)
end
local function _set_scene_clear_color_vec3(scene_handle, color3)
  _set_scene_clear_color(scene_handle, color3.x, color3.y, color3.z, 1.0)
end
local function _set_scene_clear_color_gray(scene_handle, gray)
  _set_scene_clear_color(scene_handle, gray, gray, gray, 1.0)
end
local function _get_scene_clear_color(scene_handle)
  return __other_native.__scene_interface.get_scene_clear_color(scene_handle)
end

local function _get_object_name(scene_handle, native_id)
  return __other_native.__scene_interface.get_object_name(scene_handle, native_id)
end
local function _set_object_name(scene_handle, native_id, name)
  __other_native.__scene_interface.set_object_name(scene_handle, native_id, name)
end

local function _add_tag_to_object(scene_handle, native_id, tag)
  return __other_native.__scene_interface.add_tag_to_object(scene_handle, native_id, tag)
end
local function _remove_tag_from_object(scene_handle, native_id, tag)
  return __other_native.__scene_interface.remove_tag_from_object(scene_handle, native_id, tag)
end

local function _add_component_to_object(scene_handle, native_id, component_name)
  return __other_native.__scene_interface.add_component_to_object(scene_handle, native_id, component_name)
end
local function _remove_component_from_object(scene_handle, native_id, component_name)
  return __other_native.__scene_interface.remove_component_from_object(scene_handle, native_id, component_name)
end
local function _check_if_object_has_component(scene_handle, native_id, component_name)
  return __other_native.__scene_interface.check_if_object_has_component(scene_handle, native_id, component_name)
end

local function _attach_dotnet_behavior_to_object(scene_handle, native_id, behavior_type_name)
  return __other_native.__scene_interface.attach_dotnet_behavior_to_object(scene_handle, native_id, behavior_type_name)
end

local function _attach_model_to_object(scene_handle, native_id, model_path)
  return __other_native.__scene_interface.attach_model_to_object(scene_handle, native_id, model_path)
end
local function _attach_camera_to_object(scene_handle, native_id)
  return __other_native.__scene_interface.attach_camera_to_object(scene_handle, native_id)
end
local function _attach_point_light_to_object(scene_handle, native_id, light)
  return __other_native.__scene_interface.attach_point_light_to_object(scene_handle, native_id, light)
end
local function _attach_direction_light_to_object(scene_handle, native_id, light)
  return __other_native.__scene_interface.attach_direction_light_to_object(scene_handle, native_id, light)
end

local _SceneInterface = {}
function _SceneInterface.GetActiveScene()
  return _get_active_scene()
end

function _SceneInterface.CreateSceneObject(name, position)
  if name == nil
  then
    error("Scene object name cannot be nil")
    return nil
  end

  local scene = _get_active_scene()
  if scene == nil
  then
    error("No active scene available to create scene object in")
    return nil
  end

  if position == nil
  then
    return scene.create_scene_object(name)
  else
    return scene.create_scene_object(name, position)
  end
end

function _SceneInterface.CreateParentedSceneObject(name, position, parent_id)
  if name == nil
  then
    error("Scene object name cannot be nil")
    return nil
  end

  local scene = _get_active_scene()
  if scene == nil
  then
    error("No active scene available to create scene object in")
    return nil
  end

  if parent_id == nil
  then
    return _SceneInterface.CreateSceneObject(name, position)
  else
    return scene.create_scene_object(name, position, parent_id)
  end
end

function _SceneInterface:new()
  local obj = {}
  setmetatable(obj, self)
  self.__index = self
  return obj
end

function _SceneInterface:GetNativeScenePointer()
  local scene_handle = _get_active_scene()
  if scene_handle == nil
  then
    error("No active scene available to get native pointer from")
    return nil
  end
  return scene_handle.__native_pointer
end

function _SceneInterface:CallInterfaceFunction(func, ...)
  local native_pointer = self:GetNativeScenePointer()
  if native_pointer == nil
  then
    error("No valid native scene handle available to call interface function")
    return nil
  end

  return func(native_pointer, ...)
end

function _SceneInterface:GetClearColor()
  return self:CallInterfaceFunction(_get_scene_clear_color)
end
function _SceneInterface:SetClearColor(r, g, b, a)
  self:CallInterfaceFunction(_set_scene_clear_color, r, g, b, a)
end
function _SceneInterface:SetClearColorVec4(color4)
  self:CallInterfaceFunction(_set_scene_clear_color_vec4, color4)
end
function _SceneInterface:SetClearColorVec3(color3)
  self:CallInterfaceFunction(_set_scene_clear_color_vec3, color3)
end
function _SceneInterface:SetClearColorGray(gray)
  self:CallInterfaceFunction(_set_scene_clear_color_gray, gray)
end


function _SceneInterface:GetObjectName(native_id)
  return self:CallInterfaceFunction(_get_object_name, native_id)
end
function _SceneInterface:SetObjectName(native_id, name)
  self:CallInterfaceFunction(_set_object_name, native_id, name)
end

function _SceneInterface:AddObjectTag(native_id, tag)
  return self:CallInterfaceFunction(_add_tag_to_object, native_id, tag)
end
function _SceneInterface:RemoveObjectTag(native_id, tag)
  return self:CallInterfaceFunction(_remove_tag_from_object, native_id, tag)
end

function _SceneInterface:AddComponentToObject(native_id, component_name)
  return self:CallInterfaceFunction(_add_component_to_object, native_id, component_name)
end
function _SceneInterface:RemoveComponentFromObject(native_id, component_name)
  return self:CallInterfaceFunction(_remove_component_from_object, native_id, component_name)
end
function _SceneInterface:CheckIfObjectHasComponent(native_id, component_name)
  return self:CallInterfaceFunction(_check_if_object_has_component, native_id, component_name)
end

function _SceneInterface:AttachModelToObject(native_id, model_path)
  return self:CallInterfaceFunction(_attach_model_to_object, native_id, model_path)
end
function _SceneInterface:AttachCameraToObject(native_id)
  return self:CallInterfaceFunction(_attach_camera_to_object, native_id)
end
function _SceneInterface:AttachPointLightToObject(native_id, light)
  return self:CallInterfaceFunction(_attach_point_light_to_object, native_id, light)
end
function _SceneInterface:AttachDirectionLightToObject(native_id, light)
  return self:CallInterfaceFunction(_attach_direction_light_to_object, native_id, light)
end

function _SceneInterface:AttachDotNetBehaviorToObject(native_id, behavior_type_name)
  return self:CallInterfaceFunction(_attach_dotnet_behavior_to_object, native_id, behavior_type_name)
end

local _Scene = _SceneInterface:new()
return _Scene