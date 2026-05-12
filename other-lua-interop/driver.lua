function __environment_console_init_hook()end
function __is_command(command) return Other:Console():IsCommand(command) end
function __handle_console_command(command) return Other:Console():HandleCommand(command) end
