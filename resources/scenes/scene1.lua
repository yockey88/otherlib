local MyObject = Other:SceneObject():new()
MyObject.Transform.local_position = Vec3:new(0.0, 0.0, 0.0)
MyObject.Transform.local_rotation_quat = Quat:new(0.0, 0.0, 0.0, 1.0)
MyObject.Transform.local_scale = Vec3:new(1.0, 1.0, 1.0)
MyObject.Scripts = {
  [".NET"] = {
    ["Test-Object"] = Other:DotnetTypes().Get("TestObject"),
  }
}
local my_render_comp = MyObject:AddComponent("render_component")

return {
  Objects = {
    MyObject,
  },
}