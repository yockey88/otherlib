/// canonical scene lighting for material-driven passes: the deferred shading resolve and the
/// forward transparent pass compile this against the same light/environment data, so surfaces
/// shade identically on both paths. flat include convention (stb_include has no guards) — the
/// includer pulls in camera.glsl, simulation-environment.glsl, and basic-lighting.glsl first.

uniform sampler3D OE_env_cubemap;
uniform sampler3D OE_voxel_tex;

#ifndef OE_SHADOW_MAX_STEPS
#define OE_SHADOW_MAX_STEPS 48
#endif

#ifndef OE_SHADOW_NORMAL_BIAS
#define OE_SHADOW_NORMAL_BIAS 1.5   // start offset along the surface normal, in voxels (anti-acne)
#endif

/// metal environment response tuning: the sky mirror takes reduced diffuse-AO occlusion (full
/// env.a double-darkens polished metal) and metals keep a slice of the irradiance volume as a
/// floor — real metal reflects its diffuse surroundings, and the default sky alone is dim gray
#ifndef OE_METAL_AO_BLEND
#define OE_METAL_AO_BLEND 0.5
#endif
#ifndef OE_METAL_IRRADIANCE_FLOOR
#define OE_METAL_IRRADIANCE_FLOOR 0.3
#endif

vec3 oe_environment_ambient_color(vec3 amb_col, vec3 N) {
  float up = clamp(normalize(N).y * 0.5 + 0.5, 0.0, 1.0);
  return mix(ground_color.rgb, amb_col, up);
}

vec3 oe_environment_ambient(vec3 world_pos, vec3 N) {
  vec3  amb_up = texture(OE_env_cubemap, oe_world_to_volume(world_pos)).rgb;
  return oe_environment_ambient_color(amb_up, N);
}

float oe_point_shadow(vec3 surface_pos, vec3 surface_normal, vec3 light_pos) {
  vec3 to_light = light_pos - surface_pos;
  float dist = length(to_light);
  if (dist < 1e-4) {
    return 1.0;
  }
  vec3  dir = to_light / dist;

  ivec3 dim = textureSize(OE_voxel_tex, 0);
  vec3  vsize = oe_voxel_size(dim);                       // world units per voxel
  float vlen = max(max(vsize.x, vsize.y), vsize.z);

  vec3 p = surface_pos + surface_normal * (vlen * OE_SHADOW_NORMAL_BIAS) + dir * (vlen * 0.5);
  float remaining = dist - vlen * (OE_SHADOW_NORMAL_BIAS + 1.0);
  if (remaining <= 0.0) {
    return 1.0;
  }

  int steps = int(clamp(remaining / vlen, 1.0, float(OE_SHADOW_MAX_STEPS)));
  float step_len = remaining / float(steps);

  float trans = 1.0;
  for (int i = 0; i < steps; ++i) {
    float occ = texture(OE_voxel_tex, oe_world_to_voxel(p, dim)).r;
    trans *= (1.0 - clamp(occ, 0.0, 1.0));
    if (trans < 0.01) {
      return 0.0;
    }
    p += dir * step_len;
  }
  return trans;
}

vec4 calculate_lighting(vec3 diffuse, vec3 world_position, vec3 world_normal, float specular_reflect, float metalness) {
  vec3 view_dir = normalize(camera_position.xyz - world_position);

  /// metals: no lambertian body color, specular tinted by albedo instead of white.
  /// dielectrics (metalness 0) reproduce the old math exactly.
  vec3 kd = diffuse * (1.0 - metalness);
  vec3 spec_tint = mix(vec3(1.0), diffuse, metalness);

  vec3 diffuse_specular = vec3(0);
  for (int i = 0; i < OE_num_lights; ++i) {
    if (lights[i].type == 1.f) {
      vec3 lp = lights[i].vector.xyz;
      vec3 light_dir = normalize(lp - world_position);
      vec3 diff = max(dot(world_normal, light_dir), 0.0) * kd * lights[i].color.rgb;

      vec3 halfway = normalize(light_dir + view_dir);
      float spec = pow(max(dot(world_normal, halfway), 0.0), 16.0);

      vec3 specular = lights[i].color.rgb * spec * specular_reflect * spec_tint;
      float atten = attenuate(length(lp - world_position));

      float vis = oe_point_shadow(world_position, world_normal, lp);
      diffuse_specular +=
      // vis *
      ((diff * atten) + (specular * atten));
    } else if (lights[i].type == 2.f) {
      /// directional (gpu::light::kDirection): vector.xyz points TOWARD the light — the same
      /// TO-sun convention sim env uses — with no attenuation; shadowing rides the sun-shadow
      /// factor applied to the whole accumulated term below
      vec3 light_dir = normalize(lights[i].vector.xyz);
      vec3 diff = max(dot(world_normal, light_dir), 0.0) * kd * lights[i].color.rgb;

      vec3 halfway = normalize(light_dir + view_dir);
      float spec = pow(max(dot(world_normal, halfway), 0.0), 16.0);

      diffuse_specular += diff + lights[i].color.rgb * spec * specular_reflect * spec_tint;
    }
  }

  vec4 env = texture(OE_env_cubemap, oe_world_to_volume(world_position));
  vec3 ambient = oe_environment_ambient_color(env.rgb, world_normal) * env.a;

  /// metals trade the flat ambient body for environment reflection: the mirrored procedural
  /// sky (world_max.w mirrors the sky branch's exposure scale) plus the irradiance floor
  vec3 sky_reflection = oe_sky_radiance(reflect(-view_dir, world_normal)) * world_max.w * mix(1.0, env.a, OE_METAL_AO_BLEND);
  vec3 metal_env = sky_reflection + ambient * OE_METAL_IRRADIANCE_FLOOR;

  float shadow_calc = calculate_direction_light_shadow(world_position, world_normal);
  vec3 lighting = ambient * kd + metal_env * diffuse * metalness + (1.0 - shadow_calc) * diffuse_specular;
  return vec4(lighting, 1.0);
}
