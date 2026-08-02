local Floor = Other:SceneObject():new("Floor")
Floor.Transform.local_position = Vec3:new(0.0, -5.0, 0.0)
-- Floor.Transform.local_scale = Vec3:new(10.0, 0.5, 10.0)
local floor_render = Floor:AttachModel("resources/models/floor.fbx")
local floor_material = GraphicsMaterial:new()
floor_material.diffuse_color = Vec3:new(0.2, 0.2, 0.2)
floor_material.diffuse_reflectivity = 0.5
floor_material.specular_color = Vec3:new(0.8, 0.8, 0.8)
floor_material.specular_reflectivity = 0.5
floor_material.emissivity = 0.0
floor_material.shininess = 16.0
floor_material.transparency = 0.0
floor_render:SetMaterial(floor_material)

local Donut = Other:SceneObject():new("Donut")
Donut.Transform.local_position = Vec3:new(0.0, 0.0, 0.0)
Donut.Transform.local_rotation_quat = Quat:new(0.0, 0.0, 0.0, 1.0)
Donut.Transform.local_scale = Vec3:new(1.0, 1.0, 1.0)
local donut_render = Donut:AttachModel("resources/models/spinning-torus.fbx")
local donut_material = GraphicsMaterial:new()
donut_material.diffuse_color = Vec3:new(0.01, 0.01, 0.22)
donut_material.diffuse_reflectivity = 0.5
donut_material.specular_color = Vec3:new(0.8, 0.8, 0.8)
donut_material.specular_reflectivity = 0.5
donut_material.emissivity = 0.1
donut_material.shininess = 16.0
donut_material.transparency = 0.0
donut_render:SetMaterial(donut_material)
Donut:AttachBehavior("TestBehavior");

local Light = Other:SceneObject():new("Light", Vec3:new(2.0, 4.0, 2.0))
Light.Transform.local_position = Vec3:new(0.0, 2.0, 0.0)
Light.Transform.local_scale = Vec3:new(0.1, 0.1, 0.1)
local pl1 = PointLight:new()
pl1.position = Vec3:new(2.0, 4.0, 2.0)
pl1.color = Vec4:new(1.0, 0.44, 0.77, 1.0)
Light:AttachPointLight(pl1)
-- local pl2 = PointLight:new()
-- pl2.position = Vec3:new(-2.0, 4.0, 2.0)
-- pl2.color = Vec4:new(0.6, 0.2, 0.9, 1.0)
-- Light:AttachPointLight(pl2)

-- Light:AddTag("sun")
-- local dl = DirectionLight:new()
-- dl.direction = Vec3:new(0.0, 1.0, 0.0)
-- dl.color = Vec4:new(1.0, 1.0, 1.0, 1.0)
-- Light:AttachDirectionLight(dl)

local Camera = Other:SceneObject():new("Camera", Vec3:new(0.0, 1.0, 4.5))
local cam = Camera:AttachCamera()
cam.sensitivity = 10.0
cam:Look(Vec3:new(0.0, 1.0, 4.5), Vec3:new(0.0, 0.0, 0.0))
Camera:AddTag("main-camera")

return {
  Objects = { Floor, Donut, Light, Camera }
}