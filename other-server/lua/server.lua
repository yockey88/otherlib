OtherLog.Info("Hello from server.lua!")

local server_id = nil

local server_hook = {
  OnReceiveData = function(id, data)end,
}

local http_server_hook = {
  OnHttpRequest = function(id, req)
    -- For demonstration, we just print the request method and path, and respond with a simple message.
    if id == nil then
      OtherLog.Error("Received HTTP request with nil connection ID!")
      return
    end
    if req == nil then
      OtherLog.Error(string.format("Received HTTP request with nil request data on connection %d!", id))
      return
    end
    if req.method == nil or req.path == nil then
      OtherLog.Error(string.format("Received HTTP request with missing method or path on connection %d!", id))
      return
    end

    print(string.format("Received HTTP request:")) -- %s %s", req.method, req.path))
    -- OtherLog.Info(string.format(" [HTTP SERVER] Received HTTP request: %s %s", req.method, req.path))
    -- print("Headers:")
    -- for header_name, header_value in pairs(req.headers) do
    --   print(string.format("  %s: %s", header_name, header_value))
    -- end
    -- local response = {
    --   status_code = 200,
    --   headers = { ["Content-Type"] = "text/plain" },
    --   body = "Hello from Other Server!"
    -- }
  end,
}

function InitializeHttpServer(port)
  Other:Driver():AddInterface("Other.Server", server_hook)
  Other:Driver():AddInterface("Other.HttpServer", http_server_hook)
end