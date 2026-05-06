OtherLog.Info("Hello from server.lua!")

-- local index = {
--   status_code = 200,
--   headers = { ["Content-Type"] = "text/html" },
--   body = "<div>Hello from Other Server!</div>"
-- }
local _HTML = [[
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Chris Yockey: Software Engineer</title>
    <link rel="stylesheet" href="style.css">
</head>
<body>
  <div class="main-panel-header">
    <div class="header-panel-name">
      <h1 id="header-name">Chris Yockey</h1>
      <h2 id="header-title">Software Engineer</h2>
    </div>
    <div class="header-panel-contact">
      <div class="contact-widget" id="LinkedIn-widget">
        <a href="https://www.linkedin.com/in/chris-yockey/">LinkedIn</a>
      </div>
      <div class="contact-widget" id="GitHub-widget">
        <a href="https://github.com/yockey88">Github</a>
      </div>
      <div class="contact-widget" id="Email-widget">
        <a href="mailto:chrisyockey88@gmail.com">chrisyockey88@gmail.com</a>
      </div>
    </div>
  </div>
  <div class="main-panel-about-me">
    <div class="about-me-panel"></div>
    <div class="about-me-other"></div>
  </div>
</body>
</html>
]]

local index = {
  status_code = 200,
  headers = { ["Content-Type"] = "text/html" },
  body = _HTML
}
local routes = {
  ['/'] = index --require("lua.index")
}

local function _handle_http_request(path)
  if routes[path] then
    return routes[path]
  else
    return {
      status_code = 404,
      headers = { ["Content-Type"] = "text/html" },
      body = "<div>404 Not Found</div>"
    }
  end
end


local function _get_http_server()
  local http_server_hook = {
    routes =  routes,
    OnHttpRequest = function(id, request) --, method, path, body)
      local _response = _handle_http_request(request.path)
      local response = HttpResponse:new(_response.status_code)
      if _response.headers then
        response:set_headers(_response.headers)
      end
      
      OtherLog.Info(string.format([[ [HTTP SERVER] serving request [%s%s], status-code = %d ]], request.method.name, request.path, response.status_code))
      __server_native_SendHttpResponse(id, response)
    end,
  }
  return http_server_hook
end
local http_server_hook = _get_http_server()
local server_hook = {
  OnAcceptConnection = function(id) OtherLog.Info(string.format("[SERVER] Connection %d", id)) end,
  OnReceiveData = function(id, data) end,
  OnCloseConnection = function(id) OtherLog.Info(string.format("[SERVER] Connection %d closed", id)) end,
}

function GetServerLuaInterface() return server_hook end
function GetHttpServerLuaInterface() return http_server_hook end
function InitializeHttpServer(port)
  Other:Driver():AddInterface("Other.Server", server_hook)
  Other:Driver():AddInterface("Other.HttpServer", http_server_hook)
end

