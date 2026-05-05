OtherLog.Info("Hello from server.lua!")

local index = {
  status_code = 200,
  headers = { ["Content-Type"] = "text/html" },
  body = "<div>Hello from Other Server!</div>"
}

local routes = {
  ['/'] = index
}

local function _handle_http_request(path, verb, query, body)
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
      -- print("Received HTTP request: " .. id .. " " .. request)
      if request == nil then
        OtherLog.Warn("Received nil HTTP request with id: " .. id)
        return
      end
      print("[HTTP] " .. id)
      print("[HTTP: Method] " .. request.method.name)
      print("[HTTP: Path] " .. request.path)
      print("[HTTP: Query] " .. request.query)
      print("[HTTP: Body] ")
      for i = 1, #request.body do
        io.write(string.format("%02X ", request.body[i]))
      end
      print("")
      print("[HTTP: Headers]")
      for k, v in pairs(request.headers) do
        print("  " .. k .. ": " .. v)
      end

      local _response = _handle_http_request(request.path, request.method.name, request.query, request.body)
      local response = HttpResponse:new()
      response.status_code = _response.status_code
      response.headers = _response.headers
      -- response:set_body(_response.body)
      
      __server_native_SendHttpResponse(id, response)
    end,
  }
  return http_server_hook
end
local http_server_hook = _get_http_server()
local server_hook = {
  OnAcceptConnection = function(id)end,
  OnReceiveData = function(id, data)end,
  OnCloseConnection = function(id)end,
}

function GetServerLuaInterface() return server_hook end
function GetHttpServerLuaInterface() return http_server_hook end
function InitializeHttpServer(port)
  Other:Driver():AddInterface("Other.Server", server_hook)
  Other:Driver():AddInterface("Other.HttpServer", http_server_hook)
end

