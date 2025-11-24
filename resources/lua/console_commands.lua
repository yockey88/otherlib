function send_log(level, message, source, line)
  __other_native.__log.send_log_message(level, message, source, line)
end

function log_debug(...)
  local info = debug.getinfo(3, "Sl")
  local source = info.short_src
  local line = info.linedefined
  local message = table.concat({...}, "\t")
  send_log(log_level.DEBUG, message, source, line)
end

function log_info(...)
  local info = debug.getinfo(3, "Sl")
  local source = info.short_src
  local line = info.linedefined
  local message = table.concat({...}, "\t")
  send_log(log_level.INFO, message, source, line)
end

function log_warning(...)
  local info = debug.getinfo(3, "Sl")
  local source = info.short_src
  local line = info.linedefined
  local message = table.concat({...}, "\t")
  send_log(log_level.WARNING, message, source, line)
end

function log_error(...)
  local info = debug.getinfo(3, "Sl")
  local source = info.short_src
  local line = info.linedefined
  local message = table.concat({...}, "\t")
  send_log(log_level.ERROR, message, source, line)
end

function log_fatal(...)
  local info = debug.getinfo(3, "Sl")
  local source = info.short_src
  local line = info.linedefined
  local message = table.concat({...}, "\t")
  send_log(log_level.CRITICAL, message, source, line)
end

local console = {}

function other_env_entry_hook()
  console.commands = {
    help = {
      description = "Displays this help message.",
      handler = function(args)
        local help_message = "\nAvailable commands:\n"
        for command, cmd_info in pairs(console.commands) do
          help_message = help_message .. string.format(" - %s: %s\n", command, cmd_info.description)
        end
        log_info(help_message)
      end
    },
    exit = {
      description = "Exits the Other Environment runtime.",
      handler = function(args)
        __other_native.__driver.trigger_driver_event("shutdown-requested")
      end
    },
  }
end

local function is_command(command)
  return console.commands[command] ~= nil
end

function handle_console_command(command)
  local is_cmd = is_command(command)
  if is_cmd then
    console.commands[command].handler({})  
  end
  
  return is_cmd
end