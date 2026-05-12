local function _read_file()
  local exists = __server_native_FileExists("style.css")
  if not exists then
    OtherLog.Error("style.css not found in server mount directory")
    return {}
  end

  OtherLog.Debug("style.css found, reading content")
  return __server_native_ReadFileToHttpBody("style.css")
end

return _read_file()