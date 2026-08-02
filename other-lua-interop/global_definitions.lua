LogLevel = {
  Trace = log_level.TRACE,
  Debug = log_level.DEBUG,
  Info = log_level.INFO,
  Warn = log_level.WARN,
  Error = log_level.ERROR,
  Critical = log_level.CRITICAL
};

ConsoleMessage = {
  None = console_message.CONSOLE_NONE,
  Message = console_message.CONSOLE_MESSAGE,
  Trace = console_message.CONSOLE_TRACE,
  Debug = console_message.CONSOLE_DEBUG,
  Info = console_message.CONSOLE_INFO,
  Warn = console_message.CONSOLE_WARN,
  Error = console_message.CONSOLE_ERROR,
  Command = console_message.CONSOLE_COMMAND
};

DriverState = driver_state
DriverEvent = driver_event

--- where immediate-mode draws land: Scene is depth tested inside the scene pipeline,
--- Debug renders in the editor debug overlay on top of the finished frame
DrawTarget = {
  Scene = 0,
  Debug = 1
};

