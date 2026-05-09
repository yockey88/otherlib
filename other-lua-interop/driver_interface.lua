local function _long_flag_to_short_flag(flag)
  -- turn --flag-name into -fn and --flag into -f
  return flag:gsub("-(-%a)%a+", "%1"):gsub("(%a)-(%a)%a+", "%1%2")
end

local function _deduce_open_close_type(args)
  if #args == 1
  then
    --- in this case we expect a file exists and we will open it according to it's extension
    return "file"
  elseif #args == 2
  then
    local flag = args[1]
    local stripped_flag = _Meta._string_utils.strip_leading_and_ending_whitespace(flag)
    if #stripped_flag == 0 or stripped_flag == ""
    then -- default to file if no flag provided
      return "file"
    end
    --- expect ((-f|--file) <file-path>) or ((-will|--window) (window-name|window-id))
    local flag = args[1]
    if flag == "-f" or flag == "--file"
    then
      return "file"
    elseif flag == "-w" or flag == "--window"
    then
      return "window"
    else
      return "unknown"
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
    local stripped_flag = _Meta._string_utils.strip_leading_and_ending_whitespace(flag)
    if #stripped_flag == 0 or stripped_flag == ""
    then -- default to files if no flag provided
      return "files"
    elseif #stripped_flag > 2
    then
      stripped_flag = _long_flag_to_short_flag(stripped_flag)
    end


    if stripped_flag == "-f"
    then return "files"
    elseif stripped_flag == "-ds"
    then return "driver-systems"
    elseif stripped_flag == "-w"
    then return "windows"
    elseif stripped_flag == "-s"
    then return "scenes"
    elseif stripped_flag == "-a"
    then return "assets"
    else return "unknown"
    end
  end
  return "unknown"
end

local function _deduce_project_op_type(args)
  if #args == 0 then
    return "unknown"
  end

  local operation = _Meta._string_utils.strip_leading_and_ending_whitespace(args[1])
  if operation == "--new" or operation == "-n"
  then return "new" end
  if operation == "--load" or operation == "-l"
  then return "load" end
  if operation == "--save" or operation == "-s"
  then return "save" end

  return "unknown"
end

local function _parse_project_args(args)
  local result = { type = "unknown" }
  if #args < 1 then
    return result
  end

  -- for new and load, expect a project name or path as the second argument
  local op = _Meta._string_utils.strip_leading_and_ending_whitespace(args[1])
  if op == "--load" or op == "-l" then
    if #args < 2
    then return result end

    result.type = "load"
    result.args = {
      project_name = nil,
      project_path = _Meta._string_utils.strip_leading_and_ending_whitespace(args[2]),
    }
  elseif op == "--new" or op == "-n" then
    if #args < 2
    then return result end

    result.type = "new"
    result.args = {
      project_name = _Meta._string_utils.strip_leading_and_ending_whitespace(args[2]),
      project_path = nil,
    }

    -- we also expect a path, if there is none then cwd will be used
    if #args >= 3 then
      result.args.project_path = _Meta._string_utils.strip_leading_and_ending_whitespace(args[3])
    end
  elseif op == "--save" or op == "-s" then
    result.type = "save"
    -- get project name/path from active project
  end

  return result
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
local function _add_main_menu_bar_menu_item_impl(menu_name, items)
  __other_native.__driver.add_main_menu_bar_menu_item(menu_name, items)
end

local function _validate_menu_bar_menu_items(items)
  if type(items) ~= "table"
  then
    _Meta:Console().PushError("Menu items must be provided as a table.")
    return false
  end

  for i, item in ipairs(items)
  do
    if type(item) ~= "table"
    then
      _Meta:Console().PushError(string.format("Menu item at index %d is not a table.", i))
      return false
    end

    if type(item.Name) ~= "string"
    then
      _Meta:Console().PushError(string.format("Menu item at index %d does not have a valid 'Name' field.", i))
      return false
    end

    -- nil could be if it as SubItems instead
    if item.Action ~= nil and type(item.Action) ~= "function"
    then
      _Meta:Console().PushError(string.format("Menu item at index %d does not have a valid 'Action' function.", i))
      return false
    end
    
    if item.SubItems ~= nil and type(item.SubItems) ~= "table"
    then
      _Meta:Console().PushError(string.format("Menu item at index %d has an invalid 'SubItems' field; it must be a table.", i))
      return false
    end
    if item.SubItems ~= nil
    then
      if not _validate_menu_bar_menu_items(item.SubItems)
      then
        return false
      end
    end
  end

  return true
end

local function _add_interface(interface_name, interface_table)
  return __other_native.__driver.add_interface(interface_name, interface_table)
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
    _Meta:Console().PushError("Usage: ls [options] (see ls --help for more details)")
    return {}, false
  end

  if not _get_list_arg(result, args[#args])
  then
    return {}, false
  end
  return result, true
end

function _Driver._parse_project_op_args(args)
  if #args == 0 then
    _Meta:Console().PushError("No arguments provided for project command.")
    _Meta:Console().PushError("Usage: project [options...] <arguments>... (see project --help for more details)")
    return {}, false
  end

  local result = _parse_project_args(args)
  if result.type == nil or result.type == "unknown"
  then
    _Meta:Console().PushError("Unknown arguments for project command.")
    _Meta:Console().PushError("Usage: project [options...] <arguments>... (see project --help for more details)")
    return {}, false
  end

  return result, true
end

function _Driver._parse_object_op_args(args)
  local result = {}
  if #args < 1
  then
    _Meta:Console().PushError("No arguments provided for object command.")
    _Meta:Console().PushError("Usage: object <operation> [options...] (see object --help for more details)")
    return {}, false
  end

  result.operation = _deduce_object_op_type(args)
  if result.operation == "unknown"
  then
    _Meta:Console().PushError("Unknown operation for object command.")
    _Meta:Console().PushError("Usage: object <operation> [options...] (see object --help for more details)") 
    return {}, false
  end

  -- additional argument parsing can be done here based on operation type
  result.op_table = _build_object_arg_table(result.operation, args)
  if result.op_table == nil
  then
    _Meta:Console().PushError("Invalid arguments for object command operation: " .. result.operation)
    _Meta:Console().PushError("Usage: object <operation> [options...] (see object --help for more details)")
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

function _D:_OpenClose(type, args)
  local parsed_args, success = self:_parse_open_close_args(type, args)
  if not success
  then
    return
  end

  if parsed_args.type == "file"
  then
    _Meta:Console().PushError("[TODO] open file requested: " .. parsed_args.path)
    -- local open_args = {
    --   type = "file",
    --   path = file_path
    -- }
    -- Driver.TriggerEvent("open-requested", open_args)
  elseif parsed_args.type == "window"
  then
    self.TriggerEvent(string.format("%s-driver-ui-window", type), parsed_args.window_identifier)
  end
end

function _D:_List(args)
  local parsed_args, success = self._parse_list_args(args)
  if not success
  then
    return
  end

  local event_name = "ls." .. parsed_args.type
  self.TriggerEvent(event_name)
end

function _D:_Project(args)
  local parsed_args, success = self._parse_project_op_args(args)
  if not success
  then
    return
  end

  local event_name = "project." .. parsed_args.type
  self.TriggerEvent(event_name, parsed_args.args)
end

function _D:_ObjectOpEvent(operation, op_table)
  local event_name = "object-driver-" .. operation
  if operation ~= nil and 
    (operation == "create" or operation == "destroy" or operation == "push" or operation == "info") and
     op_table.identifier ~= nil then
    self.TriggerEvent(event_name, op_table.identifier)
  end
  if operation == "pop" 
  then
    self.TriggerEvent(event_name)
  end
end

function _D:_ObjectOp(args)
  local parsed_args, success = self._parse_object_op_args(args)
  if not success
  then
    return
  end

  if parsed_args.operation == nil
  then
    _Meta:Console().PushError("No operation specified for object command")
    return
  end

  if parsed_args.op_table == nil
  then
    _Meta:Console().PushError("No operation data specified for object command")
    return
  end

  local event_name = "object-driver-" .. parsed_args.operation
  self:_ObjectOpEvent(parsed_args.operation, parsed_args.op_table)
end

function _D:_SceneOp(args)
  local parsed_args, success = self._parse_scene_op_args(args)
  if not success
  then return end

  if parsed_args.operation == "new"
  then
    local scene_name = parsed_args.scene_name
    if scene_name == nil or scene_name == ""
    then
      _Meta:Console().PushError("No scene name provided for new scene command")
      return
    end

    -- _Meta:Driver().TriggerEvent("force-load-empty-scene", _Meta._string_utils.strip_leading_and_ending_whitespace(scene_name))
  elseif parsed_args.operation == "load"
  then
    local scene_path = parsed_args.scene_path
    if scene_path == nil or scene_path == ""
    then
      _Meta:Console().PushError("No scene path provided for load scene command")
      return
    end

    _Meta:LoadScene(_Meta._string_utils.strip_leading_and_ending_whitespace(scene_path))
  elseif parsed_args.operation == "unload"
  then
    -- _Meta:Driver().TriggerEvent("scene.unload-scene")
  elseif parsed_args.operation == "info"
  then
    self.TriggerEvent("scene.request-info")
  elseif parsed_args.operation == "play" or 
         parsed_args.operation == "pause" or 
         parsed_args.operation == "stop"
  then
    self.TriggerEvent("scene.playback-command", parsed_args.operation)
  else
    _Meta:Console().PushError("Unknown scene command operation: " .. tostring(parsed_args.operation))
  end
end

function _D:OpenCommand(args)
  self:_OpenClose("open", args)
end
function _D:CloseCommand(args)
  self:_OpenClose("close", args)
end

function _D:OpenWindow(arg)
  self:_OpenClose("open", { "--window", _Meta:String().as_string(arg) })
end
function _D:CloseWindow(arg)
  self:_OpenClose("close", { "--window", _Meta:String().as_string(arg) })
end

function _D:OpenFile(arg)
  self:_OpenClose("open", { "--file", _Meta:String().as_string(arg) })
end
function _D:CloseFile(arg)
  self:_OpenClose("close", { "--file", _Meta:String().as_string(arg) })
end

function _D:ListCommand(args)
  self:_List(args)
end

function _D:ProjectCommand(args)
  self:_Project(args)
end

function _D:ObjectCommand(args)
  self:_ObjectOp(args)
end

function _D:SceneCommand(args)
  self:_SceneOp(args)
end

function _D:AddMainMenuBarMenu(menu_name, items)
  if menu_name == nil or menu_name == ""
  then
    _Meta:Console().PushError("Invalid menu name provided to AddMainMenuBarMenu")
    return
  end

  if not _validate_menu_bar_menu_items(items)
  then
    _Meta:Console().PushError("Invalid items table provided to AddMainMenuBarMenu")
    return
  end

  _add_main_menu_bar_menu_item_impl(menu_name, items)
end

function _D:BrowseFilesForProject()
  self.TriggerEvent("driver.queue-project-load")
end

function _D:AddInterface(interface_name, interface_table)
  if interface_name == nil or interface_name == ""
  then
    _Meta:Console().PushError("Invalid interface name provided to AddInterface")
    return
  end

  if type(interface_table) ~= "table"
  then
    _Meta:Console().PushError("Invalid interface table provided to AddInterface; expected a table.")
    return
  end

  _add_interface(interface_name, interface_table)
end

return _D