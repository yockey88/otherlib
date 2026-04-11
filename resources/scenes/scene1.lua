local MyObject = Other:SceneObject():new("MyObject")
MyObject.Transform.local_position = Vec3:new(0.0, -0.5, 0.0)
MyObject.Transform.local_rotation_quat = Quat:new(0.0, 0.0, 0.0, 1.0)
MyObject.Transform.local_scale = Vec3:new(1.0, 1.0, 1.0)
MyObject:AttachBehavior("TestBehavior");

-- local render_comp = MyObject:AttachModel("resources/models/spinning-torus.fbx")
-- local material = GraphicsMaterial:new()
-- material.diffuse_color = Vec3:new(0.4, 0.6, 0.8)
-- material.diffuse_reflectivity = 0.5
-- material.specular_color = Vec3:new(0.8, 0.8, 0.8)
-- material.specular_reflectivity = 0.5
-- material.emissivity = 0.1
-- material.shininess = 16.0 
-- material.transparency = 0.0
-- render_comp:SetMaterial(material)

local Light = Other:SceneObject():new("Light", Vec3:new(2.0, 4.0, 2.0))
Light:AddTag("scene-ambient-light")
Light.Transform.local_position = Vec3:new(0.0, 2.0, 0.0)
Light.Transform.local_scale = Vec3:new(0.1, 0.1, 0.1)

local dl = DirectionLight:new()
dl.direction = Vec3:new(0.0, -1.0, 0.0)
dl.color = Vec3:new(1.0, 1.0, 1.0)

Light:AttachDirectionLight(dl)

local Camera = Other:SceneObject():new("Camera", Vec3:new(0.0, 1.0, 4.5))
Camera:AddTag("main-camera")

local cam = Camera:AttachCamera()
cam.sensitivity = 0.35
cam:Look(Vec3:new(0.0, 1.0, 4.5), Vec3:new(0.0, 0.0, 0.0))

return {
  Objects = {
    MyObject,
    Light,
    Camera,
  },
}