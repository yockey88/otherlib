function _send_log_impl(level, message)
  --- get source and line from stack of where log_xxx(...) was called (up two levels)
  local info = debug.getinfo(3, "Sl") or {}
  local source = info.short_src or "unknown"
  local line = info.currentline or 0
  __other_native.__log.send_log_message(level, message, source, line)

  --- allow users to define this hook to intercept log messages
  if (__other_log_intercept_hook ~= nil) 
  then
    __other_log_intercept_hook(string.format(" [Lua] [%s] %s:%d: %s", level, source, line, message))
  end
end

function _submit_console_text_impl(message, message_type)
  __other_native.__environment_console.submit_console_text(message, message_type)
end

function _trigger_driver_event_impl(event_name, event_data)
  __other_native.__driver.trigger_driver_event(event_name, event_data)
end

function _get_active_scene()
  return __other_native.__driver.get_active_scene()
end

function _get_lua_bridge_metadata_table()
  return __lua_bridge_metadata
end

_Meta = _get_lua_bridge_metadata_table()
_Meta.__index = _Meta
function _Meta:scripts_directory_path()  return self.__paths.script_directory end
function _Meta:bridge_path()             return self.__paths.other_bridge end
function _Meta:global_definitions_path() return self.__paths.global_definitions end
function _Meta:get_core_script(name)     return string.format("%s/%s.lua", self:scripts_directory_path(), name) end

local _scripts_directory = _Meta:scripts_directory_path()
local _require_fmt_scripts_directory = _scripts_directory:gsub("/", "."):gsub("\\", ".")

_Meta._base_require_directory = _require_fmt_scripts_directory
function _Meta:get_script(name) return require(string.format("%s.%s", self._base_require_directory, name)) end
function _Meta:do_file(path)
  local full_path = string.format("%s/%s", self:scripts_directory_path(), path)
  local chunk, err = loadfile(full_path)
  if not chunk then
    error(string.format("Error loading file '%s': %s", full_path, err))
  end
  return chunk()
end

--- things people shouldn't be touching
_NativeSceneObject = __native_scene_object
_NativeTransformComponent = __native_transform_component
_NativeScriptComponent = __native_script_component
_NativeRenderComponent = __native_render_component
_NativeCameraComponent = __native_camera_component

GraphicsMaterial = __native_gpu_graphics_material
PointLight = __native_gpu_point_light
DirectionLight = __native_gpu_directional_light

Vec2 = __native_vector2
Vec3 = __native_vector3
Vec4 = __native_vector4
Quat = __native_quaternion


_Meta._string_utils = _Meta:get_script("string_utils")
_Meta._file_utils = _Meta:get_script("file_utils")
_Meta._dotnet_type_cache = _Meta:get_script("dotnet_types")
_Meta._scene_interface = _Meta:get_script("scene_interface")
_Meta._scene_object_interface = _Meta:get_script("scene_object_interface")

function _Meta:String() return self._string_utils end
function _Meta:File() return self._file_utils end
function _Meta:Scene() return self._scene_interface end
function _Meta:SceneObject() return self._scene_object_interface end
function _Meta:DotnetTypes() return self._dotnet_type_cache end

Driver = {
  TriggerEvent = function(event_name, event_data)
    _trigger_driver_event_impl(event_name, event_data)
  end,
}

CoreLog = {
  Trace = function(...)    _send_log_impl(LogLevel.Trace, ...)    end,
  Debug = function(...)    _send_log_impl(LogLevel.Debug, ...)    end,
  Info = function(...)     _send_log_impl(LogLevel.Info, ...)     end,
  Warn = function(...)     _send_log_impl(LogLevel.Warn, ...)  end,
  Error = function(...)    _send_log_impl(LogLevel.Error, ...)    end,
  Critical = function(...) _send_log_impl(LogLevel.Critical, ...) end,
}
CoreLog.Debug("[lua bridge ".. _Meta:global_definitions_path() .. "] Global Definitions Loaded")
CoreLog.Debug("[lua bridge ".. _Meta:bridge_path() .. "] Loading Other Environment Lua Bridge")

_Meta._console = _Meta:get_script("environment_console")
function _Meta:Console()
  return self._console
end

--- TODO: check if there are user commands to register from config or elsewhere and 
---          do so here

Other = _Meta