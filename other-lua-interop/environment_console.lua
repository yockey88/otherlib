_Console = {
  num_commands = 0,
  command_order = {},
  commands = {},

  PushConsoleMessage = function(message) _Meta._driver_interface.TriggerEvent("console.output", message) end,
  PushDebug = function(message) _Meta._driver_interface.TriggerEvent("console.debug", message) end,
  PushInfo = function(message) _Meta._driver_interface.TriggerEvent("console.info", message) end,
  PushWarn = function(message) _Meta._driver_interface.TriggerEvent("console.warn", message) end,
  PushError = function(message) _Meta._driver_interface.TriggerEvent("console.error", message) end,

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

  local help_message = "Available commands:\n"
  for _, name in ipairs(command_names) do
    local cmd_info = self.commands[name]
    help_message = help_message .. string.format(" - %s: %s\n", name, cmd_info.description)
  end

  self.PushConsoleMessage(help_message)
end

function _Console:ExitCommand(args)
  _Meta:Driver().TriggerEvent("shutdown-requested")
end


function _Console:ListCommand(args)
  _Meta:Driver():ListCommand(args)
end

function _Console:ProjectCommand(args)
  _Meta:Driver():ProjectCommand(args)
end

function _Console:ObjectCommand(args)
  _Meta:Driver():ObjectCommand(args)
end

function _Console:SceneCommand(args)
  _Meta:Driver():SceneCommand(args)
end

function _Console:CreateSceneCommand(args)
  if #args < 1
  then
    self.PushError("Usage: new-scene <scene-name>")
    return
  end
  local scene_name = args[1]
  _Meta:Driver().TriggerEvent("force-load-empty-scene", _Meta._string_utils.strip_leading_and_ending_whitespace(scene_name))
end

function _Console:LoadSceneCommand(args)
  if #args < 1
  then
    self.PushError("Usage: load-scene <scene-path>")
    return
  end

  _Meta:LoadScene(args[1])
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

    local command_info = self.commands[cmd]
    if #args == 1 and (args[1] == "--help" or args[1] == "-h")
    then
      if command_info.long_description == nil
      then
        self.PushConsoleMessage(self.commands[cmd].description)
        return
      else 
        self.PushConsoleMessage(command_info.long_description)
      end
    else
      -- _Meta:Driver().TriggerEvent("console.command", command)
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

  self:RegisterConsoleCommand("help", "Displays this help message", function(...) self:HelpCommand(...) end)
  self:RegisterConsoleCommand("exit", "Exits the Other Environment runtime", function(...) self:ExitCommand(...) end)
  self:RegisterConsoleCommand("clear", "Clears the console output", function(...) _Meta:Driver().TriggerEvent("console.clear") end)

  local open_close_help_message = [[
  [%s Command]
    %s files, windows, scenes, and other resources.
    If no flags are provided, the command assumes a file path is given and attempts to %s the file accordingly.
    Usage:
      %s -h|--help                           Displays this help message
      %s -f|--file   file-path               %s the specified file in the appropriate manner based on file type
      %s -w|--window window-name|window-id   %s the UI window with the given name or ID
  ]]
  local format_help_string = function(name, cmd, action)
    return string.format(open_close_help_message, name, action, cmd, cmd, cmd, action, cmd, action)
  end
  local open_help_msg = format_help_string("Open", "open", "Opens")
  local close_help_msg = format_help_string("Close", "close", "Closes")

  self:RegisterConsoleCommand("open", "Runs the open function with the specified arguments", function(...) _Meta:Driver():OpenCommand(...) end, open_help_msg)
  self:RegisterConsoleCommand("close", "Runs the close function with the specified arguments", function(...) _Meta:Driver():CloseCommand(...) end, close_help_msg)

  local ls_long_description = [[
  [ls Command]
    Prints a list that take various forms depending on the arguments.
    Default behavior is to list the contents of the current directory.
    Usage:
      ls -h|--help              Displays this help message
      ls [-f|--files]           Lists all files registered in the filesystem
      ls -ds|--driver-systems    Lists all currently loaded driver systems
      ls -s|--scenes            Lists all scenes in the scene graph
      ls -a|--assets            Lists all assets registered in the asset manager
      ls -w|--windows           Lists all registered UI windows
  ]]
  self:RegisterConsoleCommand("ls", "Prints a list of items.", function(...) _Meta:Driver():ListCommand(...) end, ls_long_description)

  local project_long_description = [[
  [project Command]
    Provides various project management functions such as creating, loading, and saving projects.
    Usage:
      project -h|--help                              Displays this help message
      project -n|--new project-name [project-path]   Creates a new empty project with the specified name at the specified path
      project -l|--load project-path                 Loads a project from the specified path
      project -s|--save [project-path]               Saves the current project to its current path, or to a new path if one is specified
  ]]
  self:RegisterConsoleCommand("project", "Manipulate, create, and manage Other Environment projects", function(...) _Meta:Driver():ProjectCommand(...) end, project_long_description)

  -- local object_long_description = [[
  -- [object Command]
  --   Performs various operations on scene objects.
  --   Usage:
  --     object (-h |--help)                               Displays this help message
  --     object <-c |--create> <object-name>               Creates a new scene object with the specified name and pushes it to the stack for further operations
  --     object <-d |--delete> [object-id|object-name]     Deletes the specified scene object, or the top object on the stack if none is specified. This also pops the object from the stack if it is the top object.
  --     object <-pu|--push> [object-id|object-name]       Pushes the specified scene object to the stack for further operations
  --     object <-po|--pop>                                Pops the top scene object from the stack
  --     object <-i |--info> [object-id|object-name]       Displays detailed information about the specified scene object,
  --                                                       or the top object on the stack if none is specified

  --   Features In Development:
  --     object transform set <position|rotation|scale> <x> <y> <z>   Sets the specified transform property of the top object on the stack
  --     object transform get <position|rotation|scale>               Gets the specified transform property of the top object on the stack
  --     object script add <script-path>                              Attaches a script to the top object on the stack
  --     object script remove <script-name>                           Removes a script from the top object on the stack
    
  -- ]]
  -- self:RegisterConsoleCommand("object", "Performs various operations on scene objects.", function(...) _Meta:Driver():ObjectCommand(...) end, object_long_description)


  local scene_long_description = [[
  [scene Command]
    Performs various operations on the current scene.
    Usage:
      scene play                               Starts or resumes scene play
      scene pause                              Pauses scene play
      scene stop                               Stops scene play and resets to the beginning

      scene -h|--help                          Displays this help message
      scene -n|--new scene-name                Creates a new empty scene with the specified name
      scene -l|--load scene-path               Loads a scene from the specified path
      scene -u|--unload                        Unloads the current scene
      scene -i|--info) [scene-name|scene-path] Displays information about the current scene or the specified scene
  ]]
  self:RegisterConsoleCommand("scene", "Performs various operations on the current scene.", function(...) _Meta:Driver():SceneCommand(...) end, scene_long_description)

  local net_long_description = [[
  [net Command]
    Controls the default networking session.
    Usage:
      net host [port]           Hosts a session on the given port (default from networking.port)
      net join <ip[:port]>      Joins a session at the given address
      net leave                 Leaves the current session
      net status                Prints session role, roster, and per-link state
  ]]
  self:RegisterConsoleCommand("net", "Controls the default networking session.", function(args) _Meta:Driver().TriggerEvent("network.command", table.concat(args, " ")) end, net_long_description)

  setmetatable(obj, self)
  return obj
end

local _C = _Console:new()
return _C