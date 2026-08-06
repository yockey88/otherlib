OtherLog.Info("Hello from server.lua!")

-- headless TCP server shell: listens on server.main-port and logs connection
-- traffic through the Other.Server interface hooks. request handling returns
-- when the session layer can carry it (networking-1).
local server_hook = {
  OnReceiveData = function(id, data)
    OtherLog.Debug(string.format("[SERVER] Connection %d received %d bytes", id, data and #data or 0))
  end,
  OnAcceptConnection = function(listening_id, connected_id)
    OtherLog.Info(string.format("[SERVER] Connection %d (from %d)", connected_id, listening_id))
  end,
  OnCloseConnection = function(id)
    OtherLog.Info(string.format("[SERVER] Connection %d closed", id))
  end,
}

function InitializeServer(port)
  Other:Driver():AddInterface("Other.Server", server_hook)
  OtherLog.Info(string.format("[SERVER] listening on port %d", port))
end
