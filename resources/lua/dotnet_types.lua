local _DotnetTypesCache = {}

function _DotnetTypesCache.Get(class_name)
  return  __other_native.__dotnet_types[class_name]
  -- return DotnetObject:new(dotnet_type)
end

return _DotnetTypesCache