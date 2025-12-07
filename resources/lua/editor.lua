local console = require("resources.lua.console")

function __environment_console_init_hook()
  -- print("Console initialized.")
end

function is_command(command) 
  return console:is_command(command)
end
function handle_console_command(command) 
  return console:handle_console_command(command)
end
