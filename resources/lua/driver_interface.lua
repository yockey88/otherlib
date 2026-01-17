local function _deduce_open_close_type(args)
  if #args == 1
  then
    --- in this case we expect a file exists and we will open it according to it's extension
    return "file"
  elseif #args == 2
  then
    --- expect ((-f|--file) <file-path>) or ((-will|--window) (window-name|window-id))
    local flag = args[1]
    if flag == "-f" or flag == "--file"
    then
      return "file"
    else if flag == "-w" or flag == "--window"
    then
      return "window"
    else
      return "unknown"
    end
    end
  end 
  return "unknown"
end

local function _deduce_list_type(args)
  if #args == 0
  then
    return "default"
  elseif #args == 1
  then
    local flag = args[1]
    if flag == "-w" or flag == "--windows"
    then
      return "windows"
    else if flag == "-f" or flag == "--files"
    then
      return "files"
    else if flag == "-s" or flag == "--scenes"
    then
      return "scenes"
    else if flag == "-a" or flag == "--assets"
    then
      return "assets"
    else
      return "unknown"
    end
    end
    end
    end
  end 
  return "unknown"
end

local function _get_path_from_args(arg)
  local path = _Meta._string_utils.strip_leading_and_ending_whitespace(arg)
  if not _Meta._file_utils.Exists(path)
  then
    return nil
  end
  return path
end

local function _get_open_close_arg(result_table, arg)
  if arg == nil or arg == ""
  then
    error("Invalid argument provided to _get_open_close_arg")
  end

  if result_table.type == nil or result_table.type == "unknown"
  then
    error("Invalid result table type provided to _get_open_close_arg")
  end

  if result_table.type == "file"
  then
    result_table.path = _get_path_from_args(arg)
    if result_table.path == nil
    then
      _Meta:Console().PushError("File does not exist: " .. arg)
      return false
    end
  elseif result_table.type == "window"
  then
    result_table.window_identifier = _Meta._string_utils.strip_leading_and_ending_whitespace(arg)
  end
  return true
end

local function _get_list_arg(result_table, arg)
  if arg == nil or arg == ""
  then
    error("Invalid argument provided to _get_list_arg")
  end

  if result_table.type == nil or result_table.type == "unknown"
  then
    error("Invalid result table type provided to _get_list_arg")
  end

  -- no additional args needed for now
  return true
end

local function _trigger_driver_event_impl(event_name, event_data)
  __other_native.__driver.trigger_driver_event(event_name, event_data)
end
local function _process_driver_event_impl(event)
  __other_native.__driver.process_driver_event(event)
end

local _Driver = {
  State = {
    Stopped = DriverState.STOPPED,
    Initializing = DriverState.INITIALIZING,
    Running = DriverState.RUNNING,
    Paused = DriverState.PAUSED,
    ShuttingDown = DriverState.SHUTTING_DOWN,

    NUM_DRIVER_STATES = DriverState.NUM_STATES,
  },
  Event = {
    Start = DriverEvent.START,
    Ready = DriverEvent.READY,
    Pause = DriverEvent.PAUSE,
    Resume = DriverEvent.RESUME,
    Stop = DriverEvent.STOP,

    NUM_DRIVER_EVENTS = DriverEvent.NUM_EVENTS,
  },
  TriggerEvent = function(event_name, event_data)
    _trigger_driver_event_impl(event_name, event_data)
  end,
  ProcessEvent = function(event)
    _process_driver_event_impl(event)
  end,
}

function _Driver:new()
  local new_driver = {}
  setmetatable(new_driver, self)
  self.__index = self
  return new_driver
end

function _Driver:_parse_open_close_args(cmd_name, args)
  if #args < 1
  then
    _Meta:Console().PushError("Usage: " .. cmd_name .. " [options...] <file-path|window-name|window-id>")
    return {}, false
  end

  if args[1] == nil or args[1] == ""
  then
    _Meta:Console().PushError("No arguments provided for " .. cmd_name .. " command.")
    return {}, false
  end
  if args[2] ~= nil and args[2] == ""
  then
    _Meta:Console().PushError("Invalid second argument provided for " .. cmd_name .. " command.")
    return {}, false
  end

  local result = {}
  result.type = _deduce_open_close_type(args)
  if result.type == "unknown"
  then
    _Meta:Console().PushError("Unknown arguments for " .. cmd_name .. " command.")
    return {}, false
  end

  if not _get_open_close_arg(result, args[#args])
  then
    return {}, false
  end
  return result, true
end

function _Driver._parse_list_args(args)
  local result = { type = "default" }
  if #args == 0
  then
    return result, true
  end

  result.type = _deduce_list_type(args)
  if result.type == "unknown"
  then
    _Meta:Console().PushError("Unknown arguments for list command.")
    return {}, false
  end

  if not _get_list_arg(result, args[#args])
  then
    return {}, false
  end
  return result, true
end

local _D = _Driver:new()
return _D