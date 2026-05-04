local _FileUtils = {}

function _FileUtils.Exists(path)
  local file = io.open(path, "r")
  if file ~= nil
  then
    io.close(file)
    return true
  else
    return false
  end
end

return _FileUtils