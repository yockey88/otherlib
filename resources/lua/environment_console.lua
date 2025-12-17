_Console = {
  num_commands = 0,
  command_order = {},
  commands = {},

  PushMessage = function(message, message_type)
    _submit_console_text_impl(message, message_type)
  end,
  PushConsoleMessage = function(message)
    _submit_console_text_impl(message, ConsoleMessage.Message)
  end,
  PushTrace = function(message)
    _submit_console_text_impl(message, ConsoleMessage.Trace)
  end,
  PushDebug = function(message)
    _submit_console_text_impl(message, ConsoleMessage.Debug)
  end,
  PushInfo = function(message)
    _submit_console_text_impl(message, ConsoleMessage.Info)
  end,
  PushWarn = function(message)
    _submit_console_text_impl(message, ConsoleMessage.Warn)
  end,
  PushError = function(message)
    _submit_console_text_impl(message, ConsoleMessage.Error)
  end,

  GetCommandName = function(name)
    local stripped = _Meta._string_utils.strip_leading_and_ending_whitespace(name)
    return stripped:match("^(%S+)")
  end,
}

function _Console:HelpCommand(args)
  local command_names = {}
  for i = 1, self.num_commands do
    command_names[i] = self.command_order[i]
  end

  local help_message = "\nAvailable commands:\n"
  for _, name in ipairs(command_names) do
    local cmd_info = self.commands[name]
    help_message = help_message .. string.format(" - %s: %s\n", name, cmd_info.description)
  end

  self.PushConsoleMessage(help_message)
end

function _Console:ExitCommand(args)
  Driver.TriggerEvent("shutdown-requested")
end

function _Console:CreateSceneCommand(args)
  if #args < 1
  then
    self.PushError("Usage: new-scene <scene-name>")
    return
  end
  local scene_name = args[1]
  Driver.TriggerEvent("force-load-empty-scene", _Meta._string_utils.strip_leading_and_ending_whitespace(scene_name))
end

function _Console:LoadSceneCommand(args)
  if #args < 1
  then
    self.PushError("Usage: load-scene <scene-path>")
    return
  end

  local path = _Meta._string_utils.strip_leading_and_ending_whitespace(args[1])
  if not _Meta._file_utils.Exists(path)
  then
    self.PushError("Scene file does not exist: " .. args[1])
    return
  end

  Driver.TriggerEvent("force-load-scene", path)
end

local function _parse_open_close_args(cmd_name, console, args)
  if #args < 1
  then
    console.PushError("Usage: " .. cmd_name .. " <arguments...>")
    return {}, false
  end

  local result = {
    type = "file",
  }

  if #args == 1
  then
    --- in this case we expect a file exists and we will open it according to it's extension
  elseif #args == 2
  then
    --- expect ((-f|--file) <file-path>) or ((-will|--window) (window-name|window-id))
    local flag = args[1]
    if flag == "-f" or flag == "--file"
    then
      result.type = "file"
    else if flag == "-w" or flag == "--window"
    then
      result.type = "window"
    else
      console.PushError("Unknown flag for " .. cmd_name .. " command: " .. flag)
      return {}, false
    end
    end
  end 

  if result.type == "file"
  then
    result.path = _Meta._string_utils.strip_leading_and_ending_whitespace(args[#args])
    if not _Meta._file_utils.Exists(result.path)
    then
      console.PushError("File does not exist: " .. result.path)
      return {}, false
    end
  elseif result.type == "window"
  then
    result.window_identifier = _Meta._string_utils.strip_leading_and_ending_whitespace(args[#args])
  end
  return result, true
end

function _Console:OpenCommand(args)
  local parsed_args, success = _parse_open_close_args("open", self, args)
  if not success
  then
    return
  end

  if parsed_args.type == "file"
  then
    self.PushError("[TODO] open file requested: " .. parsed_args.path)
    -- local open_args = {
    --   type = "file",
    --   path = file_path
    -- }
    -- Driver.TriggerEvent("open-requested", open_args)
  elseif parsed_args.type == "window"
  then
    print("ui-window : " .. parsed_args.window_identifier)
    Driver.TriggerEvent("open-driver-ui-window", parsed_args.window_identifier)
  end
end

function _Console:CloseCommand(args)
  local parsed_args, success = _parse_open_close_args("close", self, args)
  if not success
  then
    return
  end

  if parsed_args.type == "file"
  then
    self.PushError("[TODO] close file requested: " .. parsed_args.path)
    -- local close_args = {
    --   type = "file",
    --   path = file_path
    -- }
    -- Driver.TriggerEvent("close-requested", close_args)
  elseif parsed_args.type == "window"
  then
    Driver.TriggerEvent("close-driver-ui-window", parsed_args.window_identifier)
  end

  -- Driver.TriggerEvent("close-requested", close_args)
end

function _Console:OpenWindowCommand(args)
  if #args < 1
  then
    self.PushError("Usage: open-window <window-name|window-id>")
    return
  end

  local new_cmd = { "--window", args[1] }
  self:OpenCommand(new_cmd)
end

function _Console:CloseWindowCommand(args)
  if #args < 1
  then
    self.PushError("Usage: close-window <window-name|window-id>")
    return
  end

  local new_cmd = { "--window", args[1] }
  self:CloseCommand(new_cmd)
end

function _Console:IsCommand(command)
  return self.commands[self.GetCommandName(command)] ~= nil
end

function _Console:HandleCommand(command)
  local is_cmd = self:IsCommand(command)
  if is_cmd 
  then
    local command_line = _Meta._string_utils.split_string_list(command)

    local cmd = self.GetCommandName(command)
    local args = {}
    for i = 1, #command_line do
      table.insert(args, command_line[i])
    end

    print("cmd:", cmd)
    print("args:", table.concat(args, ", "))
    local command_info = self.commands[cmd]

    if #args == 1 and (args[1] == "--help" or args[1] == "-h") and command_info.long_description ~= nil
    then
      self.PushConsoleMessage(command_info.long_description)
    else
      self.commands[self.GetCommandName(command)].handler(args)
    end
  end

  return is_cmd
end

function _Console:RegisterConsoleCommand(command_name, description, handler, long_description)
  self.num_commands = self.num_commands + 1
  self.command_order[self.num_commands] = command_name
  self.commands[command_name] = {
    description = description,
    long_description = long_description,
    handler = handler
  }
end

_Console.__index = _Console
function _Console:new()
  local obj = {}
  
  self:RegisterConsoleCommand(":?", "Displays this help message", function(...) self:HelpCommand(...) end)
  self:RegisterConsoleCommand(":e", "Exits the Other Environment runtime", function(...) self:ExitCommand(...) end)
  self:RegisterConsoleCommand("help", "Displays this help message", function(...) self:HelpCommand(...) end)
  self:RegisterConsoleCommand("exit", "Exits the Other Environment runtime", function(...) self:ExitCommand(...) end)

  self:RegisterConsoleCommand("clear", "Clears the console output", function(...) Driver.TriggerEvent("clear-console-output") end)

  self:RegisterConsoleCommand("new-scene", "Creates a new empty scene", function(...) self:CreateSceneCommand(...) end)
  self:RegisterConsoleCommand("load-scene", "Loads a scene from a specified path", function(...) self:LoadSceneCommand(...) end)

  local open_close_help_message = [[

  [%s Command]
    %s files, windows, scenes, and other resources.
    If no flags are provided, the command assumes a file path is given and attempts to %s the file accordingly.
    Usage:
      %s (-h|--help)                             Displays this help message
      %s (-f|--file)? <file-path>                %s the specified file in the appropriate manner based on file type
      %s (-w|--window) <window-name|window-id>   %s the UI window with the given name or ID
  ]]
  local format_help_string = function(name, cmd, action)
    return string.format(open_close_help_message, name, action, cmd, cmd, cmd, action, cmd, action)
  end
  local open_help_msg = format_help_string("Open", "open", "Opens")
  local close_help_msg = format_help_string("Close", "close", "Closes")

  self:RegisterConsoleCommand("open", "Runs the open function with the specified arguments", function(...) self:OpenCommand(...) end, open_help_msg)
  self:RegisterConsoleCommand("close", "Runs the close function with the specified arguments", function(...) self:CloseCommand(...) end, close_help_msg)
  self:RegisterConsoleCommand("open-window", "Opens the UI window the given name", function(...) self:OpenWindowCommand(...) end)
  self:RegisterConsoleCommand("close-window", "Closes the UI window the given name", function(...) self:CloseWindowCommand(...) end)

  setmetatable(obj, self)
  return obj
end

local _C = _Console:new()
return _C