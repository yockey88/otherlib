print("Hello from editor-rc.lua!")

Other:Driver():AddMainMenuBarMenu(
  "File",
  {
    {
      Name = "Project",
      SubItems = {
        { Name = "New Project", Action = function() Other:Driver().TriggerEvent("project.new-project") end },
        { Name = "Open Project", Action = function() Other:Driver().TriggerEvent("project.open-project") end },
        { Name = "Save Project", Action = function() Other:Driver().TriggerEvent("project.save-project") end },
      }
    },
    -- { 
    --   Name = "Options", SubItems = {
    --     { Name = "Graphics", Action = function() Other:Driver():TriggerEvent("ui.open-window", "graphics-settings") end },
    --     { Name = "Input", Action = function() Other:Driver():TriggerEvent("ui.open-window", "input-settings") end },
    --   }
    -- },
    { Name = "Exit", Action = function() Other:Console():ExitCommand() end },
  }
)

Other:Driver():OpenWindow("console")
Other:Driver():OpenWindow("viewport")
-- Other:Driver():OpenWindow("asset-browser")
Other:Driver():OpenWindow("scene-hierarchy")

-- Other:Console():RegisterConsoleCommand("testcmd", "A test command that prints its arguments", function(...)
--   print("Test command executed with arguments:")
-- end, "Usage: testcmd [args...]\nPrints the provided arguments to the console.")
