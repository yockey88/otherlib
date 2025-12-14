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
  if not _Meta._file_utils.file_exists(path)
  then
    self.PushError("Scene file does not exist: " .. args[1])
    return
  end

  Driver.TriggerEvent("force-load-scene", path)
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
    self.commands[self.GetCommandName(command)].handler(args)
  end

  return is_cmd
end

function _Console:RegisterConsoleCommand(command_name, description, handler)
  self.num_commands = self.num_commands + 1
  self.command_order[self.num_commands] = command_name
  self.commands[command_name] = {
    description = description,
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

  self:RegisterConsoleCommand("new-scene", "Creates a new empty scene", function(...) self:CreateSceneCommand(...) end)
  self:RegisterConsoleCommand("load-scene", "Loads a scene from a specified path", function(...) self:LoadSceneCommand(...) end)

  setmetatable(obj, self)
  return obj
end

local _C = _Console:new()
return _C