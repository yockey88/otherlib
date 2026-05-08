local _DotnetTypesCache = {}

function _DotnetTypesCache.Get(class_name)
  return  nil -- __other_native.__dotnet_types[class_name]
end

return _DotnetTypesCache