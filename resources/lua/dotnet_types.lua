local _DotnetTypesCache = {}

function _DotnetTypesCache.Get(class_name)
  return  __other_native.__dotnet_types[class_name]
end

return _DotnetTypesCache