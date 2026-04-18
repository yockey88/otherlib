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
    return "files"
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

local function _deduce_object_op_type(args)
  if #args < 1
  then
    return "unknown"
  end
  
  local operation = _Meta._string_utils.strip_leading_and_ending_whitespace(args[1])
  if operation == "--create" or operation == "-c" 
  then return "create" end
  if operation == "--delete" or operation == "-d" 
  then return "delete" end
  if operation == "--push"   or operation == "-pu" 
  then return "push" end
  if operation == "--pop"    or operation == "-po" 
  then return "pop" end
  if operation == "--info"   or operation == "-i"
  then return "info" end

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

local function _get_id_or_name(arg)
  if arg == nil or arg == ""
  then
    return nil, nil
  end
  
  local is_number = tonumber(arg) ~= nil
  local is_name = arg ~= nil and not is_number and type(arg) == "string"
  if not is_number and not is_name
  then
    return nil, nil
  end
  
  if is_number
  then
    return tonumber(arg), nil
  else
    return nil, arg
  end
end

local function _build_object_arg_table(operation, args)
  local result_table = {}
  if operation == "create"
  -- expect name, optional parent id, optional components
  then
    result_table.name = args[2] or "NewObject"
    result_table.parent_id = args[3] or nil
    result_table.components = {}
    for i = 4, #args do
      table.insert(result_table.components, args[i])
    end
  elseif operation == "delete"
  -- expect object id
  then
    result_table.object_id = args[2] or nil
  elseif operation == "push"
  -- expect object id or component name
  then
    local num, str = _get_id_or_name(args[2])
    if num == nil and str == nil
    then
      return nil, false
    end

    if num ~= nil
    then
      result_table.identifier = num
    elseif str ~= nil
    then
      result_table.identifier = str
    end
  elseif operation == "pop"
  then
    -- nothing to do
  elseif operation == "info"
  -- expect object id
  then
    local num, str =_get_id_or_name(args[2])
    if num == nil and str == nil
    then
      result_table.identifier = "<stack>"
    elseif num ~= nil
    then
      result_table.identifier = num
    elseif str ~= nil
    then
      result_table.identifier = str
    end
  else
    return nil, false
  end
  return result_table, true
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
  local result = { type = "files" }
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

function _Driver._parse_object_op_args(args)
  local result = {}
  if #args < 1
  then
    _Meta:Console().PushError("Usage: object <operation> [options...]")
    return {}, false
  end

  result.operation = _deduce_object_op_type(args)
  if result.operation == "unknown"
  then
    _Meta:Console().PushError("Unknown operation for object command.")
    return {}, false
  end

  -- additional argument parsing can be done here based on operation type
  result.op_table = _build_object_arg_table(result.operation, args)
  if result.op_table == nil
  then
    _Meta:Console().PushError("Failed to parse arguments for object command.")
    return {}, false
  end

  return result, true
end

function _Driver._parse_scene_op_args(args)
  local result = {}
  if #args < 1
  then
    _Meta:Console().PushError("Usage: scene <operation> [options...]")
    return {}, false
  end

  local first_arg = _Meta._string_utils.strip_leading_and_ending_whitespace(args[1])
  if first_arg == "--new" or first_arg == "-n"
  then
    result.operation = "new"
    result.scene_name = args[2] or nil
  elseif first_arg == "--load" or first_arg == "-l"
  then
    result.operation = "load"
    result.scene_path = args[2] or nil
  elseif first_arg == "--unload" or first_arg == "-ul"
  then
    result.operation = "unload"
  elseif first_arg == "--info" or first_arg == "-i"
  then
    result.operation = "info"
  
  elseif first_arg == "play" or first_arg == "pause" or first_arg == "stop"
  then
    result.operation = first_arg
  else
    _Meta:Console().PushError("Unknown operation for scene command.")
    return {}, false
  end

  return result, true
end

local _D = _Driver:new()
return _D