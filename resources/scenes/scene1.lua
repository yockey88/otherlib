local MyObject = Other:SceneObject():new("MyObject")
MyObject.Transform.local_position = Vec3:new(0.0, -0.5, 0.0)
MyObject.Transform.local_rotation_quat = Quat:new(0.0, 0.0, 0.0, 1.0)
MyObject.Transform.local_scale = Vec3:new(1.0, 1.0, 1.0)
-- MyObject.Scripts = {
--   [".NET"] = {
--     ["Test-Object"] = Other:DotnetTypes().Get("TestObject"),
--   }
-- }

-- local Floor = Other:SceneObject():new("Floor")
-- Floor.Transform.local_position = Vec3:new(0.0, -3.0, 0.0)
-- Floor.Transform.local_scale = Vec3:new(10.0, 1.0, 10.0)
-- Floor.Transform.local_rotation_quat = Quat:new(0.0, 0.0, 0.0, 1.0)

local material = GraphicsMaterial:new()
material.diffuse_color = Vec3:new(0.4, 0.6, 0.8)
material.diffuse_reflectivity = 0.5
material.specular_color = Vec3:new(0.8, 0.8, 0.8)
material.specular_reflectivity = 0.5
material.emissivity = 0.1
material.shininess = 16.0 
material.transparency = 0.0

local floor_material = GraphicsMaterial:new()
floor_material.diffuse_color = Vec3:new(0.7, 0.3, 0.4)
floor_material.diffuse_reflectivity = 0.2
floor_material.specular_color = Vec3:new(0.5, 0.5, 0.5)
floor_material.specular_reflectivity = 0.3
floor_material.emissivity = 0.0
floor_material.shininess = 8.0
floor_material.transparency = 0.0

local render_comp = MyObject:AttachModel("resources/models/spinning-torus.fbx")
render_comp:SetMaterial(material)
-- local floor_render = Floor:AttachModel("resources/models/cube.fbx")
-- floor_render:SetMaterial(floor_material)

local Light = Other:SceneObject():new("Light", Vec3:new(2.0, 4.0, 2.0))
Light:AddTag("scene-ambient-light")
Light.Transform.local_position = Vec3:new(0.0, 2.0, 0.0)
Light.Transform.local_scale = Vec3:new(0.1, 0.1, 0.1)

local pl = PointLight:new()
pl.light_position = Vec3:new(0.0, 5.0, 0.0)
pl.color = Vec3:new(1.0, 1.0, 1.0)

local dl = DirectionLight:new()
dl.direction = Vec3:new(0.0, -1.0, 0.0)
dl.color = Vec3:new(1.0, 1.0, 1.0)

Light:AttachPointLight(pl)
Light:AttachDirectionLight(dl)

local Camera = Other:SceneObject():new("Camera", Vec3:new(0.0, 1.0, 4.5))
Camera:AddTag("main-camera")

local cam = Camera:AttachCamera()
cam.sensitivity = 0.35
cam:Look(Vec3:new(0.0, 1.0, 4.5), Vec3:new(0.0, 0.0, 0.0))

return {
  Objects = {
    MyObject,
    -- Floor,
    Light,
    Camera,
  },
}