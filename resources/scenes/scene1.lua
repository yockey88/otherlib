local MyObject = Other:SceneObject():new("MyObject")
MyObject.Transform.local_position = Vec3:new(0.0, 0.0, 0.0)
MyObject.Transform.local_rotation_quat = Quat:new(0.0, 0.0, 0.0, 1.0)
MyObject.Transform.local_scale = Vec3:new(1.0, 1.0, 1.0)
MyObject.Scripts = {
  [".NET"] = {
    ["Test-Object"] = Other:DotnetTypes().Get("TestObject"),
  }
}


local render_comp =  MyObject:AttachModel("resources/models/spinning-torus.fbx")
if render_comp == nil
then
  CoreLog.Error("Failed to attach model to MyObject")
else
  local material = GraphicsMaterial:new()
  material.diffuse_color = Vec3:new(0.4, 0.6, 0.8)
  material.diffuse_reflectivity = 0.5
  material.specular_color = Vec3:new(0.8, 0.8, 0.8)
  material.specular_reflectivity = 0.5
  material.emissivity = 0.1
  material.shininess = 16.0 
  material.transparency = 0.0
  render_comp:SetMaterial(material)
end


local Light = Other:SceneObject():new("Light", Vec3:new(2.0, 4.0, 2.0))
Light:AddTag("scene-ambient-light")
Light.Transform.local_position = Vec3:new(0.0, 2.0, 0.0)
Light.Transform.local_scale = Vec3:new(0.1, 0.1, 0.1)

local pl = PointLight:new()
pl.light_position = Vec3:new(0.0, 5.0, 0.0)
pl.color = Vec3:new(1.0, 1.0, 1.0)
Light:AttachPointLight(pl)

local dl = DirectionLight:new()
dl.direction = Vec3:new(0.0, -1.0, 0.0)
dl.color = Vec3:new(1.0, 1.0, 1.0)
Light:AttachDirectionLight(dl)

--   gpu::point_light& light_plight = get_active_scene()->add_component<gpu::point_light>(&light_obj);
--   light_plight.light_position = glm::vec3(0.f, 5.f, 0.f);
--   light_plight.color = glm::vec3(1.f, 1.f, 1.f);

--   gpu::directional_light& light_dlight = get_active_scene()->add_component<gpu::directional_light>(&light_obj);
--   light_dlight.direction = glm::vec3(0.f, -1.f, 0.f);
--   light_dlight.color = glm::vec3(1.f, 1.f, 1.f);

local Camera = Other:SceneObject():new("Camera", Vec3:new(0.0, 1.0, 4.5))
Camera:AddTag("main-camera")

local cam = Camera:AttachCamera()
cam.sensitivity = 0.35
cam:Look(Vec3:new(0.0, 1.0, 4.5), Vec3:new(0.0, 0.0, 0.0))

-- camera_component& cam = get_active_scene()->add_component<camera_component>(&camera_obj);
-- cam.camera.sensitivity = 0.35f;
-- cam.camera.look({ 0.f, 1.f, 4.5f }, { 0.f, 0.f, 0.f });

return {
  Objects = {
    MyObject,
    -- Light,
    Camera,
  },
}