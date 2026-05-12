local function _read_file()
  -- local exists = __server_native_FileExists("favicon.ico")
  -- if not exists then
  --   OtherLog.Error("favicon.ico not found in server mount directory")
  --   return {}
  -- end
  OtherLog.Debug("no favicon for now")
  return {} --__server_native_ReadFileToHttpBody("favicon.ico")
end

return _read_file()