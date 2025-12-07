local console = { }
function console:new() 
  local instance = {}
  console.num_commands = 0
  console.command_order = {}
  console.commands = {}
  setmetatable(instance, self)
  self.__index = self
  return instance
end

local function strip_leading_and_ending_whitespace(str)
  return str:match("^%s*(.-)%s*$")
end

local function get_command_name(str)
  local stripped = strip_leading_and_ending_whitespace(str)
  return stripped:match("^(%S+)")
end

local function split_string_list(str)
  if str == nil or str == "" 
  then
    return {}
  end

  local stripped = strip_leading_and_ending_whitespace(str)
  local first_word_end = stripped:find("%s")
  if first_word_end == nil 
  then
    return {}
  end
  
  local args_str = stripped:sub(first_word_end + 1)
  local args = {}

  local current = ""
  local in_quote = false
  local i = 1
  
  while i <= #args_str 
  do
    local c = args_str:sub(i,i)

    if c == '"' 
    then
      in_quote = not in_quote
    elseif c == ' ' and not in_quote 
    then
      if current ~= "" 
      then
        table.insert(args, current)
        current = ""
      end
    else
      current = current .. c
    end
    i = i + 1
  end
  
  if current ~= "" 
  then
    table.insert(args, current)
  end

  return args
end

function console:is_command(command)
  return self.commands[get_command_name(command)] ~= nil
end

function console:handle_console_command(command)
  local is_cmd = self:is_command(command)
  if is_cmd 
  then
    local command_line = split_string_list(command)
    
    local cmd = get_command_name(command)
    local args = {}
    for i = 1, #command_line do
      table.insert(args, command_line[i])
    end


    -- print("cmd:", cmd)
    -- print("args:", table.concat(args, ", "))
    self.commands[get_command_name(command)].handler(self, args)  
  end
  
  return is_cmd
end

local function help_command(console, args)
  local command_names = {}
  for i = 1, console.num_commands do
    command_names[i] = console.command_order[i]
  end
  
  local help_message = "\nAvailable commands:\n"
  for _, name in ipairs(command_names) do
    local cmd_info = console.commands[name]
    help_message = help_message .. string.format(" - %s: %s\n", name, cmd_info.description)
  end

  __other_native.__environment_console.submit_console_text(help_message, console_message.CONSOLE_MESSAGE) 
end

local function open_project_command(console, args)
  if #args ~= 1 then 
    CoreLog.log_error("Usage: oproj <project_path>")
    return
  end

  __other_native.__driver.set_event_user_data("open-project", args[1])
  __other_native.__driver.trigger_driver_event("open-project")
end

local function edit_project_command(console, args)
  if #args ~= 1 then 
    CoreLog.log_error("Usage: edit <project_path>")
    return
  end

  __other_native.__driver.set_event_user_data("edit-project", args[1])
  __other_native.__driver.trigger_driver_event("edit-project")
end

local function register_console_command(console, command_name, description, handler)  
  console.num_commands = console.num_commands + 1
  console.command_order[console.num_commands] = command_name
  console.commands[command_name] = {
    description = description,
    handler = handler
  }
end

local function register_builtin_commands(console)
  register_console_command(console, "help", "Displays this help message.", help_command)

  -- register_console_command(console, "oproj", "Opens the specified Other Project.", open_project_command)
  -- register_console_command(console, "edit", "Edits the specified Other Project.", edit_project_command)
  register_console_command(console, "exit", "Exits the Other Environment runtime.", function(console, args)
    __other_native.__driver.trigger_driver_event("shutdown-requested")
  end)
end

local console_instance = console:new()
register_builtin_commands(console_instance)
return console_instance