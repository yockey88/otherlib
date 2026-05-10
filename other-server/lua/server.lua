OtherLog.Info("Hello from server.lua!")

local SC = require("lua.status-codes")
local H = require("lua.headers")
local routes = {
  ['favicon.ico'] = {
    headers = { ["Content-Type"] = "image/x-icon" },
    body = require("lua.favicon")
  },
  ['style.css'] = {
    headers = { ["Content-Type"] = "text/css" },
    body = require("lua.style")
  },
  ['/'] = {
    headers = { ["Content-Type"] = "text/html" },
    body = require("lua.index")
  }
}

local function _handle_http_request(path)
  if routes[path] then
    return {
      status_code = routes[path].status_code or SC.OK,
      headers = routes[path].headers or { ["Content-Type"] = "text/plain" },
      body = routes[path].body or ""
    }
  else
    OtherLog.Info(string.format(" [HTTP SERVER] no route found for path [%s], returning 404", path))
    return {
      status_code = SC.NotFound,
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

      response:add_header("Connection", "closed")
      if _response.headers then
        response:set_headers(_response.headers)
      end
      if _response.body then
        if type(_response.body) == "string" then
          response:set_body_content(_response.body, _response.headers["Content-Type"] or "text/plain")
        elseif type(_response.body) == "table" then
          -- assume it's a byte array
          response:set_body(_response.body, _response.headers["Content-Type"] or "application/octet-stream")
        end
      end

      OtherLog.Info(string.format([[ [HTTP SERVER] serving request [%s %s], status-code = %d ]], request.method.name, request.path, response.status_code))
      __server_native_SendHttpResponse(id, response)
      OtherLog.Debug(string.format([[   - response sent for request [%s %s] ]], request.method.name, request.path))
    end,
  }
  return http_server_hook
end
local http_server_hook = _get_http_server()
local server_hook = {
  OnAcceptConnection = function(listening_id, connected_id) OtherLog.Info(string.format("[SERVER] Connection %d (from %d)", connected_id, listening_id)) end,
  OnReceiveData = function(id, data) end,
  OnCloseConnection = function(id) OtherLog.Info(string.format("[SERVER] Connection %d closed", id)) end,
}

function InitializeHttpServer(port)
  Other:Driver():AddInterface("Other.Server", server_hook)
  Other:Driver():AddInterface("Other.HttpServer", http_server_hook)
end