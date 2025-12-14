local MyObject = Other:SceneObject():new()
MyObject.Transform.local_position = Vec3:new(0.0, 0.0, 0.0)
MyObject.Transform.local_rotation_quat = Quat:new(0.0, 0.0, 0.0, 1.0)
MyObject.Transform.local_scale = Vec3:new(1.0, 1.0, 1.0)
MyObject.Scripts = {
  [".NET"] = {
    ["Test-Object"] = Other:DotnetTypes().Get("TestObject"),
  }
}

function OnSceneLoad()
  CoreLog.Info("My Object Initialized")
end

function OnSceneActivate()
end

function OnSceneFixedUpdate(fixed_delta_time)
end

function OnSceneUpdate(delta_time)
end

function OnSceneLateUpdate(delta_time)
end

function OnSceneDeactivate()
end

function OnSceneUnload()
end

return {
  Objects = {
    ["My Object"] = MyObject,
  }
}