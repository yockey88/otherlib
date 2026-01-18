local socket = {}
function socket:new()
  local obj = {}
  setmetatable(obj, self)
  self.__index = self
  return obj
end

function socket.create_udp_stream(port)
  -- return __other_native.__network_ctx.create_udp_stream(port)
  CoreLog.log_trace("socket.create_udp_stream is not implemented")
  return nil
end

local sock = socket:new()
return sock