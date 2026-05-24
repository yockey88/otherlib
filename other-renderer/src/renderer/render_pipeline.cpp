/**
 * \file renderer/render_pipeline.cpp
 **/
#include "renderer/render_pipeline.hpp"

#include <algorithm>
#include <string>

#include <glm/glm.hpp>

#include "gpu_resource/renderer_resource.hpp"
#include "renderer/camera.hpp"
#include "renderer/render_graph.hpp"
#include "renderer/renderer.hpp"
#include "renderer/resource_tag.hpp"

#include "gpu_structs.hpp"
#include "pipeline_definition.hpp"

namespace other {
  namespace {

    // clang-format off
    constexpr float kQuadVertices[] = {
      -1.0f,  1.0f,  0.0f, 1.0f,
      -1.0f, -1.0f,  0.0f, 0.0f,
       1.0f, -1.0f,  1.0f, 0.0f,
      -1.0f,  1.0f,  0.0f, 1.0f,
       1.0f, -1.0f,  1.0f, 0.0f,
       1.0f,  1.0f,  1.0f, 1.0f,
    };
    // clang-format on

  }  // namespace

  void render_pipeline::initialize_pipeline(renderer* renderer) {
    renderer_ptr = renderer;
    graph = arena_allocator<render_graph>{}.allocate(renderer_ptr);

    create_resources_from_def();
    build_tag_maps();

    graph->start_pipeline();
    build_passes_from_def();
    graph->end_pipeline();

    validate();
  }

  void render_pipeline::shutdown_pipeline() {
    destroy_resources();

    arena_allocator<render_graph>{}.free(graph);
    graph = nullptr;

    renderer_ptr = nullptr;
  }

  bool render_pipeline::reload(pipeline_definition&& new_def) {
    if (renderer_ptr == nullptr) {
      CORE_LOG_ERROR("Cannot reload pipeline — not initialized.");
      return false;
    }

    renderer* r = renderer_ptr;
    shutdown_pipeline();
    definition = std::move(new_def);
    initialize_pipeline(r);
    return valid;
  }

  void render_pipeline::prepare_frame(render_data* data) {
    PROFILE_SECTION("render_pipeline::prepare_frame");
    frame_render_data = data;
    if (frame_render_data == nullptr) {
      /// possible if scene is empty
      return;
    }

    auto& reg = renderer_ptr->get_tag_registry();
    for (const auto& [_, named] : buffer_resources) {
      if (named.tag.is_none()) {
        continue;
      }

      if (auto binder = reg.find(named.tag)) {
        (binder)(*this, *data, named.handle);
      }
    }
    for (const auto& [_, named] : texture_resources) {
      if (named.tag.is_none()) {
        continue;
      }

      if (auto binder = reg.find(named.tag)) {
        (binder)(*this, *data, named.handle);
      }
    }

    apply_lighting_uniforms(*data);
  }

  void render_pipeline::render_frame(renderer* renderer_ptr) {
    OTHER_ASSERT(renderer_ptr != nullptr, "Renderer pointer must not be null.");
    PROFILE_SECTION("render_pipeline::render_frame");

    auto& g = graph->get_graph();
    const auto& execs = graph->get_executors();
    const auto& sort = graph->get_topological_sort();

    if (sort.empty()) {
      return;
    }

    for (const natural_t id : sort) {
      auto node_itr = g.nodes.find(id);
      OTHER_ASSERT(node_itr != g.nodes.end(), "Node with id {} not found in graph.", id);

      auto& n = node_itr->second;
      const auto* pass = n.pass;
      auto exec_itr = execs.find(pass->id);
      OTHER_ASSERT(exec_itr != execs.end(), "Executor for pass {} not found.", id);

      n.start_pass(renderer_ptr);
      exec_itr->second(*renderer_ptr, &n, pass->user_data);
      n.end_pass(renderer_ptr);
    }
  }

  ImTextureID render_pipeline::get_final_output_texture_id() {
    if (!screen_texture_handle.has_value() || !get_renderer()->resource_exists(*screen_texture_handle)) {
      return 0;
    }
    return get_renderer()->get_resource<texture>(*screen_texture_handle).get_imgui_texture_id();
  }

  resource_handle render_pipeline::get_screen_texture() const {
    OTHER_ASSERT(screen_texture_handle.has_value(), "Screen texture handle is not set.");
    return *screen_texture_handle;
  }

  frame_resources render_pipeline::get_frame_resources() const {
    return frame_resources{
      .tagged_buffers = tagged_buffer_handles,
      .tagged_textures = tagged_texture_handles,
    };
  }

  void render_pipeline::upload_buffer(resource_handle handle, const void* data, size_t size) {
    if (!get_renderer()->resource_exists(handle)) {
      CORE_LOG_ERROR("Cannot upload to buffer [{}] — resource does not exist.", handle);
      return;
    }
    get_renderer()->get_resource<gpu_buffer>(handle).set_data(data, size).finalize_buffer();
  }

  void render_pipeline::upload_to_handle(resource_handle handle, const void* data, size_t size) {
    get_renderer()->get_resource<gpu_buffer>(handle).set_data(data, size).finalize_buffer();
  }

  opt<resource_handle> render_pipeline::find_buffer_by_name(const std::string_view name) const {
    natural_t name_hash = FNV(name);
    auto itr = buffer_resources.find(name_hash);
    if (itr == buffer_resources.end()) {
      return std::nullopt;
    }
    return itr->second.handle;
  }

  opt<resource_handle> render_pipeline::find_texture_by_name(const std::string_view name) const {
    natural_t name_hash = FNV(name);
    auto itr = texture_resources.find(name_hash);
    if (itr == texture_resources.end()) {
      return std::nullopt;
    }
    return itr->second.handle;
  }

  opt<resource_handle> render_pipeline::find_tagged(resource_tag tag) const {
    if (auto itr = tagged_buffer_handles.find(tag); itr != tagged_buffer_handles.end()) {
      return itr->second;
    }
    if (auto itr = tagged_texture_handles.find(tag); itr != tagged_texture_handles.end()) {
      return itr->second;
    }
    return std::nullopt;
  }

  shader* render_pipeline::get_pass_shader(const std::string_view pass_name) {
    auto& g = graph->get_graph();
    auto itr = std::ranges::find_if(g.nodes, [&](const auto& pair) {
      return pair.second.pass->name == pass_name;
    });

    if (itr == g.nodes.end()) {
      CORE_LOG_ERROR("Shader pass [{}] not found in graph.", pass_name);
      return nullptr;
    }

    if (!itr->second.pass->shader_handle.has_value()) {
      return nullptr;
    }
    return &get_renderer()->get_resource<shader>(*itr->second.pass->shader_handle);
  }

  resource_handle render_pipeline::get_quad_mesh_handle() const {
    OTHER_ASSERT(quad_mesh_handle.has_value(), "Render pipeline does not have quad mesh for screen texture!");
    return *quad_mesh_handle;
  }

  void render_pipeline::apply_uniforms(shader& s, const std::map<std::string, value>& uniforms) {
    for (const auto& [name, val] : uniforms) {
      switch (val.type()) {
        case value_type::INT8: s.set_uniform(name, (int8_t)val); break;
        case value_type::UINT8: s.set_uniform(name, (uint8_t)val); break;
        case value_type::INT16: s.set_uniform(name, (int16_t)val); break;
        case value_type::UINT16: s.set_uniform(name, (uint16_t)val); break;
        case value_type::INT32: s.set_uniform(name, (int32_t)val); break;
        case value_type::UINT32: s.set_uniform(name, (uint32_t)val); break;
        case value_type::INT64: s.set_uniform(name, (int64_t)val); break;
        case value_type::UINT64: s.set_uniform(name, (uint64_t)val); break;
        case value_type::FLOAT: s.set_uniform(name, (float)val); break;
        case value_type::VEC3: {
          glm::vec3 v = val;
          s.set_uniform(name, v);
        } break;
        case value_type::VEC4: {
          glm::vec4 v = val;
          s.set_uniform(name, v);
        } break;
        case value_type::MAT4: {
          glm::mat4 v = val;
          s.set_uniform(name, v);
        } break;
        default:
          CORE_LOG_WARN("Unsupported uniform value_type {} for '{}'.", static_cast<int>(val.type()), name);
          break;
      }
    }
  }

  bool render_pipeline::has_shadow_map_pass() const {
    return definition.shadow_map_pass_name.has_value();
  }

  void render_pipeline::clear_shadow_map_pass_name() {
    definition.shadow_map_pass_name = std::nullopt;
  }

  shader* render_pipeline::get_shadow_map_pass() {
    return get_pass_shader(*definition.shadow_map_pass_name);
  }

  bool render_pipeline::has_shading_pass_name() const {
    return definition.shading_pass_name.has_value();
  }

  shader* render_pipeline::get_shading_pass() {
    return get_pass_shader(*definition.shading_pass_name);
  }

  bool render_pipeline::has_light_space_matrix_uniform() const {
    return definition.light_space_matrix_uniform_name.has_value();
  }

  std::string render_pipeline::light_space_matrix_uniform() const {
    return *definition.light_space_matrix_uniform_name;
  }

  void render_pipeline::apply_lighting_uniforms(const render_data& data) {
    glm::mat4 light_space_matrix = glm::mat4(1.0f);
    glm::vec3 light_pos = glm::vec3(1.f, 4.f, 1.f);

    if (definition.shadow_map_pass_name.has_value() && data.scene_ambient_light != nullptr) {
      if (!definition.light_space_matrix_uniform_name.has_value()) {
        CORE_LOG_ERROR("Light space matrix uniform name not defined in pipeline definition. Cannot set light space matrix for shadow mapping.");
        definition.shadow_map_pass_name = std::nullopt;  // avoid trying to set it every frame if it's not defined
      }

      float near_plane = 1.0f, far_plane = 10.f;
      glm::mat4 light_projection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);

      /// tiny shift to avoid nans
      glm::vec3 light_target = glm::vec3(0.0f, 0.0f, 0.0f);
      glm::mat4 light_view = glm::lookAt(light_pos, light_target, glm::vec3(0.f, 1.f, 0.f));

      light_space_matrix = light_projection * light_view;
      get_pass_shader(*definition.shadow_map_pass_name)
        ->bind()
        .set_uniform(*definition.light_space_matrix_uniform_name, light_space_matrix)
        .unbind();
    }

    if (definition.shading_pass_name.has_value()) {
      shader* shading_shader = get_pass_shader(*definition.shading_pass_name);
      if (shading_shader != nullptr) {
        int32_t num_point = static_cast<int32_t>(
          data.point_lights.size() > gpu::kMaxPointLights ? gpu::kMaxPointLights : data.point_lights.size()
        );
        int32_t num_dir = static_cast<int32_t>(data.scene_ambient_light != nullptr ? 1 : 0);

        shading_shader->bind()
          .set_uniform("OE_light_space_matrix", light_space_matrix)
          .set_uniform("OE_light_position", light_pos)
          .set_uniform("OE_num_point_lights", num_point)
          .set_uniform("OE_num_direction_lights", num_dir)
          .unbind();
      }
    }
  }

  void render_pipeline::override_pass_executor(const std::string_view pass_name, executor_fn&& fn) {
    executor_overrides[std::string{ pass_name }] = std::move(fn);
  }

  void render_pipeline::create_resources_from_def() {
    for (const auto& buf_def : definition.buffers) {
      resource_handle handle = gpu_buffer::create(buf_def.name, buf_def.type, buf_def.usage);
      natural_t name_hash = FNV(buf_def.name);

      auto [itr, inserted] = buffer_resources.insert({
        name_hash,
        named_resource{ .name = buf_def.name, .handle = handle, .tag = buf_def.tag },
      });
      OTHER_ASSERT(inserted, "Duplicate buffer resource name [{}] in pipeline definition.", buf_def.name);
      CORE_LOG_DEBUG("Created buffer resource [{}] with handle {}.", buf_def.name, handle);
    }

    glm::ivec2 window_size = get_renderer()->get_window_size();

    for (const auto& tex_def : definition.textures) {
      glm::ivec2 size = tex_def.use_window_size ? window_size : tex_def.fixed_size;

      resource_handle handle;
      if (tex_def.type == texture::tex_type::TEXTURE_CUBE) {
        handle = cube_map::create(tex_def.name, tex_def.format, size.x, size.y);
      } else if (tex_def.filters.has_value() && tex_def.wraps.has_value()) {
        handle = texture::create(tex_def.name, tex_def.type, tex_def.format, *tex_def.filters, *tex_def.wraps, size.x, size.y);
      } else {
        handle = texture::create(tex_def.name, tex_def.type, tex_def.format, size.x, size.y);
      }

      natural_t name_hash = FNV(tex_def.name);
      auto [itr, inserted] = texture_resources.insert({
        name_hash,
        named_resource{ .name = tex_def.name, .handle = handle, .tag = tex_def.tag },
      });
      OTHER_ASSERT(inserted, "Duplicate texture resource name [{}] in pipeline definition.", tex_def.name);
      CORE_LOG_DEBUG("Created texture resource [{}] with handle {}.", tex_def.name, handle);
    }

    for (const auto& shader_def : definition.shaders) {
      resource_handle handle;
      if (shader_def.geometry_path.has_value()) {
        handle = shader::create(shader_def.name, shader_def.vertex_path, *shader_def.geometry_path, shader_def.fragment_path, shader_def.defines);
      } else if (shader_def.compute_path.has_value()) {
        handle = shader::create(shader_def.name, *shader_def.compute_path, shader_def.defines);
      } else {
        handle = shader::create(shader_def.name, shader_def.vertex_path, shader_def.fragment_path, shader_def.defines);
      }
      shader_handles[shader_def.name] = handle;
      CORE_LOG_DEBUG("Created shader resource [{}] with handle {}.", shader_def.name, handle);
    }

    bool needs_quad = false;
    for (const auto& pass : definition.passes) {
      if (pass.executor.name == "fullscreen_quad") {
        needs_quad = true;
        break;
      }
    }

    if (!needs_quad) {
      return;
    }

    quad_mesh_handle = get_renderer()->create_resource("__pipeline_quad_mesh", resource_type::MESH);
    get_renderer()
      ->get_resource<mesh>(*quad_mesh_handle)
      .set_primitive_type(mesh::primitive_type::TRIANGLES)
      .add_attribute("position", mesh::attribute_type::FLOAT, 2, 0)
      .add_attribute("tex_coords", mesh::attribute_type::FLOAT, 2, 2)
      .upload_vertex_buffer("quad_vertices", 6, kQuadVertices, sizeof(kQuadVertices))
      .finalize_mesh();
  }

  void render_pipeline::build_tag_maps() {
    for (const auto& [hash, res] : buffer_resources) {
      if (res.tag != resource_tag::none() && !tagged_buffer_handles.contains(res.tag)) {
        tagged_buffer_handles[res.tag] = res.handle;
      }
    }
    for (const auto& [hash, res] : texture_resources) {
      if (res.tag != resource_tag::none() && !tagged_texture_handles.contains(res.tag)) {
        tagged_texture_handles[res.tag] = res.handle;
      }
      if (res.tag == resource_tag(resource_tag::kScreenTag)) {
        screen_texture_handle = res.handle;
      }
    }
  }

  void render_pipeline::build_passes_from_def() {
    for (const auto& pass_def : definition.passes) {
      /// find the shader for this pass
      opt<resource_handle> shader_handle = get_shader_handle(pass_def.shader_name);
      if (!shader_handle.has_value()) {
        CORE_LOG_ERROR("Shader [{}] for pass [{}] not found among pipeline shaders.", pass_def.shader_name, pass_def.name);
        continue;
      }

      glm::ivec2 size = resolve_size(pass_def.use_window_size, pass_def.fixed_size);

      auto builder = graph->start_pass(pass_def.name, shader_handle, pass_def.pass_type, size, pass_def.create_framebuffer);
      build_pass(pass_def, builder);
    }
  }

  void render_pipeline::validate() {
    for (resource_tag tag : definition.required_tags) {
      if (!tagged_buffer_handles.contains(tag) && !tagged_texture_handles.contains(tag)) {
        // CORE_LOG_ERROR("Pipeline [{}] requires tag [{}] but no resource provides it.", definition.name, resource_tag_to_string(tag));
        valid = false;
        return;
      }
    }

    valid = graph->is_valid();
    if (!valid) {
      CORE_LOG_ERROR("Pipeline [{}] has valid resources but render graph is invalid.", definition.name);
    }
  }

  void render_pipeline::destroy_resources() {
    for (const auto& [hash, res] : buffer_resources) {
      get_renderer()->destroy_resource(res.handle);
    }
    for (const auto& [hash, res] : texture_resources) {
      get_renderer()->destroy_resource(res.handle);
    }
    for (const auto& [name, handle] : shader_handles) {
      get_renderer()->destroy_resource(handle);
    }
    if (quad_mesh_handle.has_value()) {
      get_renderer()->destroy_resource(*quad_mesh_handle);
      quad_mesh_handle = std::nullopt;
    }

    buffer_resources.clear();
    texture_resources.clear();
    shader_handles.clear();
    tagged_buffer_handles.clear();
    tagged_texture_handles.clear();
    screen_texture_handle = std::nullopt;
  }

  void render_pipeline::build_pass(const pipeline_pass_definition& pass_def, render_graph::pass_builder& builder) {
    if (pass_def.clear_color.has_value()) {
      builder.set_clear_color(*pass_def.clear_color);
    }

    uint32_t curr_texture_slot = 0;
    for (const auto& ref : pass_def.inputs) {
      if (is_buffer_resource(ref.resource_name)) {
        auto handle = find_buffer_by_name(ref.resource_name);
        OTHER_ASSERT(handle.has_value(), "Input resource [{}] for pass [{}] not found as either buffer or texture.", ref.resource_name, pass_def.name);
        builder.buffer_resource(*handle, ref.binding, READ);
      } else {
        auto handle = find_texture_by_name(ref.resource_name);
        OTHER_ASSERT(handle.has_value(), "Input resource [{}] for pass [{}] not found as either buffer or texture.", ref.resource_name, pass_def.name);
        builder.texture_resource(*handle, curr_texture_slot++, ref.attachment, READ);
      }
    }

    for (const auto& ref : pass_def.outputs) {
      auto handle = find_texture_by_name(ref.resource_name);
      OTHER_ASSERT(handle.has_value(), "Output resource [{}] for pass [{}] not found as either buffer or texture.", ref.resource_name, pass_def.name);
      builder.texture_resource(*handle, 0, ref.attachment, WRITE);
    }

    /// set up executor and check for runtime override
    auto executor = make_executor(pass_def);
    if (executor == nullptr) {
      CORE_LOG_ERROR("Failed to create render pass executor for pass {}! invalid executor: {}", pass_def.name, pass_def.executor.name);
      return;
    }

    auto override_itr = executor_overrides.find(pass_def.name);
    if (override_itr != executor_overrides.end()) {
      executor = override_itr->second;
    }

    builder.execution_callback(std::move(executor));
    builder.end_pass();
  }

  renderer* render_pipeline::get_renderer() const {
    OTHER_ASSERT(graph != nullptr, "Render graph is not initialized. Cannot retrieve renderer.");
    return graph->get_renderer();
  }

  glm::ivec2 render_pipeline::resolve_size(bool use_window, const glm::ivec2& fixed) const {
    return use_window ?
      get_renderer()->get_window_size() :
      fixed;
  }

  bool render_pipeline::is_buffer_resource(const std::string_view name) const {
    return buffer_resources.contains(FNV(name));
  }

  render_pipeline::executor_fn render_pipeline::make_executor(const pipeline_pass_definition& pass) {
    OTHER_ASSERT(renderer_ptr != nullptr, "Renderer is null in make_executor!");

    const std::string& name = pass.executor.name;
    OTHER_ASSERT(!name.empty(), "pipeline '{}' / pass '{}': executor name is empty", definition.name, pass.name);

    // per-pipeline override wins
    if (auto it = executor_overrides.find(pass.name); it != executor_overrides.end()) {
      return it->second;
    }

    if (name.contains(":")) {
      return renderer_ptr->attempt_executor_resolution(name, pass, this);
    }

    auto& reg = renderer_ptr->get_executor_registry();
    auto factory = reg.find(name);
    OTHER_ASSERT(factory, "pipeline '{}' / pass '{}': no executor registered for '{}'", definition.name, pass.name, name);
    return (factory)(pass, this);
  }

  opt<resource_handle> render_pipeline::get_shader_handle(const std::string_view shader_name) const {
    if (auto shader_itr = shader_handles.find(shader_name.data()); shader_itr != shader_handles.end()) {
      return shader_itr->second;
    }
    return std::nullopt;
  }

}  // namespace other