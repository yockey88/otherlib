print("Hello from editor-rc.lua!")
Other:Driver():OpenWindow("console")
Other:Driver():OpenWindow("viewport")
-- Other:Driver():OpenWindow("scene-hierarchy")
Other:Driver():OpenWindow("asset-browser")

-- Other:Console():RegisterConsoleCommand("testcmd", "A test command that prints its arguments", function(...)
--   print("Test command executed with arguments:")
-- end, "Usage: testcmd [args...]\nPrints the provided arguments to the console.")

Other:LoadScene("resources/scenes/scene1.lua")