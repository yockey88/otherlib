local console = {
  commands = {
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
    }
  }
}