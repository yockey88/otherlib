/**
 * \file renderer/render_pipeline.cpp
 **/
#include "renderer/render_pipeline.hpp"

#include <algorithm>
#include <string>

#include <glm/glm.hpp>

#include "thread/thread_safety.hpp"

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

    ostd::vector<uint8_t> read_seed_texture_file(const filepath& path) {
      OTHER_ASSERT(std::filesystem::exists(path), "Seed texture file does not exist: {}", path.string());
      std::ifstream file(path, std::ios::binary);
      OTHER_ASSERT(file.is_open(), "Failed to open seed texture file: {}", path.string());
      return ostd::vector<uint8_t>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    }

  }  // namespace

  void render_pipeline::initialize_pipeline(renderer* renderer) {
    renderer_ptr = renderer;
    graph = arena_allocator<render_graph>{}.allocate(renderer_ptr);

    /// stale packs must not survive a reload — the layout itself may have changed
    material_pack_cache.clear();
    default_material_pack.clear();

    create_resources_from_def();
    build_tag_maps();

    graph->start_pipeline();
    build_passes_from_def();
    graph->end_pipeline();

    build_pass_runtimes();

    validate();

    if (valid && !definition.display_texture_name.empty()) {
      auto handle_opt = find_texture_by_name(definition.display_texture_name);
      if (!handle_opt.has_value()) {
        CORE_LOG_ERROR("Pipeline '{}' specifies display texture '{}', but no such resource was found.", definition.name, definition.display_texture_name);
      } else {
        screen_texture_handle = *handle_opt;
      }
    }
  }

  void render_pipeline::shutdown_pipeline() {
    destroy_resources();

    material_pack_cache.clear();
    default_material_pack.clear();

    destroy_pass_runtimes();

    arena_allocator<render_graph>{}.free(graph);
    graph = nullptr;

    renderer_ptr = nullptr;
  }

  bool render_pipeline::has_pass(natural_t pass_id) const {
    return pass_runtimes.find(pass_id) != pass_runtimes.end();
  }

  pass_runtime& render_pipeline::get_pass_runtime(natural_t pass_id) {
    auto itr = pass_runtimes.find(pass_id);
    OTHER_ASSERT(itr != pass_runtimes.end(), "Pass runtime with id {} not found.", pass_id);
    return itr->second;
  }

  void render_pipeline::rebuild() {
    graph->end_pipeline();
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

    bind_frame_resources(*data);
  }

  void render_pipeline::bind_frame_resources(const render_data& data) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("render_pipeline::pump_pipeline_per_frame_bindings");

    auto& reg = renderer_ptr->get_binding_registry();

    std::unordered_set<uint64_t> visited;
    for (auto& [pass_id, runtime] : pass_runtimes) {
      OTHER_ASSERT(runtime.def != nullptr, "pass_runtime[{}] has null def", pass_id);

      for (size_t i = 0; i < runtime.def->bindings.size(); ++i) {
        const auto& bd = runtime.def->bindings[i];
        if (bd.scope != binding_scope::PER_FRAME || bd.tag.is_none()) {
          continue;
        }

        auto handle_opt = find_tagged(bd.tag);
        // clang-format off
        OTHER_ASSERT(handle_opt.has_value(), "pipeline '{}' pass '{}' binding '{}': no resource for tag '{}' but a producer is declared", 
                     definition.name, runtime.def->name, bd.name, bd.tag.value());
        // clang-format on

        const resource_handle handle = *handle_opt;
        const uint64_t key = (uint64_t(bd.tag.value()) << 32) | handle.id;
        if (!visited.insert(key).second) {
          runtime.state.per_frame_handles[i] = handle;
          continue;
        }

        auto binder = reg.find_per_frame(bd.tag);
        if (binder == nullptr) {
          auto* resolver = renderer_ptr->get_pass_executor_resolver();
          if (resolver != nullptr) {
            binder = resolver->resolve_frame_binder("default", bd.tag);
          }
        }

        if (binder == nullptr) {
          // some tags (kScreenTag) legitimately have no producer
          runtime.state.per_frame_handles[i] = handle;
          continue;
        }

        binder(*this, data, handle);
        runtime.state.per_frame_handles[i] = handle;
      }
    }
  }

  void render_pipeline::bind_draw_resources(pass_runtime& runtime, const render_data& data, size_t draw_index) {
    ASSERT_MAIN_THREAD();
    PROFILE_SECTION("render_pipeline::bind_draw_resources");
    auto& reg = renderer_ptr->get_binding_registry();
    auto& api = renderer_ptr->rendering()->api();
    OTHER_ASSERT(api != nullptr, "Rendering API is null in bind_draw_resources.");

    for (size_t i = 0; i < runtime.def->bindings.size(); ++i) {
      const auto& bd = runtime.def->bindings[i];
      if (bd.scope != binding_scope::PER_DRAW_CALL) {
        continue;
      }

      auto& ring = runtime.state.per_draw_rings[i];
      OTHER_ASSERT(ring.ring_buffer.id != 0, "pass '{}' binding '{}': per-draw ring not allocated", runtime.def->name, bd.name);
      // clang-format off
      OTHER_ASSERT(ring.head + ring.element_size <= ring.capacity, "pass '{}' binding '{}': per-draw ring exhausted at draw {} (head={}, element={}, capacity={})",
                   runtime.def->name, bd.name, draw_index, ring.head, ring.element_size, ring.capacity);
      // clang-format on

      /// material slices pack against this pipeline's declared layout — the pipeline owns the
      ///  shader ABI, so the fill can't come from a pipeline-blind registry producer
      if (bd.tag.value() == resource_tag::kMaterialTag && definition.materials.has_value()) {
        std::span<uint8_t> slice{ ring.cpu_staging + ring.head, ring.element_size };
        fill_material_slice(slice, data, draw_index);
        api->buffer_range(ring.ring_buffer, ring.binding_point, ring.head, ring.element_size, ring.cpu_staging + ring.head);
        ring.head += ring.stride;
        continue;
      }

      auto producer = reg.find_per_draw(bd.tag);
      if (producer == nullptr) {
        auto* resolver = renderer_ptr->get_pass_executor_resolver();
        OTHER_ASSERT(resolver != nullptr, "pass '{}' binding '{}': no producer for tag '{}' and no resolver available", runtime.def->name, bd.name, bd.tag.value());

        producer = resolver->resolve_draw_binder("default", bd.tag);
      }
      if (producer == nullptr) {
        OTHER_ASSERT(false, "pass '{}' binding '{}': no producer for tag '{}'", runtime.def->name, bd.name, bd.tag.value());
        continue;
      }

      std::span<uint8_t> slice{ ring.cpu_staging + ring.head, ring.element_size };
      producer(data, draw_index, slice);
      api->buffer_range(ring.ring_buffer, ring.binding_point, ring.head, ring.element_size, ring.cpu_staging + ring.head);
      ring.head += ring.stride;
    }
  }

  bool render_pipeline::pass_uses_material_binding(const pass_runtime& runtime) const {
    if (!definition.materials.has_value()) {
      return false;
    }
    for (const auto& bd : runtime.def->bindings) {
      if (bd.scope == binding_scope::PER_DRAW_CALL && bd.tag.value() == resource_tag::kMaterialTag) {
        return true;
      }
    }
    return false;
  }

  void render_pipeline::apply_material_sampler_uniforms(const frame_node* node) {
    OTHER_ASSERT(node != nullptr, "Frame node must not be null in apply_material_sampler_uniforms.");
    if (!definition.materials.has_value() || !node->pass->shader_handle.has_value()) {
      return;
    }

    auto& sh = renderer_ptr->get_resource<shader>(*node->pass->shader_handle);
    for (const auto& slot : definition.materials->texture_slots) {
      sh.set_uniform(slot.uniform, static_cast<int32_t>(slot.unit));
    }
  }

  void render_pipeline::bind_material_textures(const render_data& data, size_t draw_index) {
    if (!definition.materials.has_value() || definition.materials->texture_slots.empty()) {
      return;
    }

    renderer_backend* backend = renderer_ptr->rendering();
    const material* mat = draw_index < data.draw_materials.size() ? data.draw_materials[draw_index] : nullptr;
    for (const auto& slot : definition.materials->texture_slots) {
      natural_t texture_hash = 0;
      if (mat != nullptr) {
        if (const auto it = mat->texture_hashes.find(slot.name_hash); it != mat->texture_hashes.end()) {
          texture_hash = it->second;
        }
      }

      resource_handle handle = texture_hash != 0 ? backend->get_texture(texture_hash) : resource_handle{};
      if (handle.id == 0) {
        /// always bind something: an untextured slot samples a 1x1 stand-in and the shader
        ///  math degenerates to the params — zero shader variants
        const bool normal_slot = slot.name == "normal";
        handle = backend->get_fallback_texture(normal_slot ? renderer_backend::fallback_texture::FLAT_NORMAL : renderer_backend::fallback_texture::WHITE);
      }
      renderer_ptr->get_resource<texture>(handle).bind(slot.unit);
    }
  }

  void render_pipeline::fill_material_slice(std::span<uint8_t> slice, const render_data& data, size_t draw_index) {
    const material_layout& layout = *definition.materials;
    OTHER_ASSERT(slice.size() >= static_cast<size_t>(layout.element_size) * layout.instance_capacity,
                 "material ring slice is smaller than the layout block ({} < {})", slice.size(), layout.element_size * layout.instance_capacity);

    const material* mat = draw_index < data.draw_materials.size() ? data.draw_materials[draw_index] : nullptr;
    const ostd::vector<uint8_t>& blob = packed_blob_for(mat);

    const draw_call& call = data.draw_calls[draw_index];
    OTHER_ASSERT(call.instance_count <= gpu::kMaxMaterials, "instance count {} exceeds the per-draw slot cap {}", call.instance_count, gpu::kMaxMaterials);
    const uint32_t instances = std::min<uint32_t>(call.instance_count, layout.instance_capacity);
    const bool has_tints = draw_index < data.draw_tints.size();
    for (uint32_t i = 0; i < instances; ++i) {
      std::span<uint8_t> dst = slice.subspan(static_cast<size_t>(i) * layout.element_size, layout.element_size);
      std::ranges::copy(blob, dst.begin());
      if (has_tints) {
        layout.fold_base_color_tint(dst, data.draw_tints[draw_index].tints[i]);
      }
    }
  }

  const ostd::vector<uint8_t>& render_pipeline::packed_blob_for(const material* mat) {
    const material_layout& layout = *definition.materials;
    if (mat == nullptr) {
      if (default_material_pack.size() != layout.element_size) {
        default_material_pack.resize(layout.element_size);
        layout.pack(nullptr, default_material_pack);
      }
      return default_material_pack;
    }

    if (const auto it = material_pack_cache.find(mat->key); it != material_pack_cache.end() && it->second.revision == mat->revision) {
      return it->second.blob;
    }

    material_pack_entry& entry = material_pack_cache[mat->key];
    entry.revision = mat->revision;
    entry.blob.resize(layout.element_size);
    layout.pack(mat, entry.blob, [&](std::string_view param_name, std::string_view reason) {
      CORE_LOG_WARN("material '{}' under pipeline '{}': param '{}' {}", mat->name, definition.name, param_name, reason);
    });
    return entry.blob;
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

    if (frame_render_data == nullptr) {
      return;
    }

    for (const natural_t id : sort) {
      PROFILE_SECTION("render_pipeline::render_frame--render_pass");
      auto node_itr = g.nodes.find(id);
      OTHER_ASSERT(node_itr != g.nodes.end(), "Node with id {} not found in graph.", id);

      frame_node& n = node_itr->second;
      const render_pass* pass = n.pass;
      OTHER_ASSERT(pass != nullptr, "Node {} has null pass", id);

      auto runtime_itr = pass_runtimes.find(pass->id);
      OTHER_ASSERT(runtime_itr != pass_runtimes.end(), "Pass runtime for pass id {} not built — was build_pass_runtimes() called?", pass->id);
      pass_runtime& runtime = runtime_itr->second;

      auto exec_itr = execs.find(pass->id);
      OTHER_ASSERT(exec_itr != execs.end(), "Executor for pass {} (id {}) not found.", pass->name, pass->id);
      const render_graph::pass_executor& exec = exec_itr->second;

      frame_binding_view bv{
        .defs = std::span{ runtime.def->bindings },
        .per_frame_handles = std::span{ runtime.state.per_frame_handles },
        .per_draw_offsets = {},
      };
      pass_diagnostics diag{};

      const uint32_t iters = std::max<uint32_t>(runtime.def->iterations_per_frame, 1u);

      for (uint32_t iter = 0; iter < iters; ++iter) {
        PROFILE_SECTION("render_pipeline::render_frame--render_pass--iteration");
        diag.mark(iter == 0 ? "pass:begin" : "pass:iter");

        n.start_pass(renderer_ptr, &runtime);
        if (n.pass->shader_handle.has_value() && frame_render_data != nullptr) {
          const auto& env = frame_render_data->simulation_environment;
          glm::vec3 to_sun = glm::normalize(-1.f * glm::vec3(env.sun_direction));
          glm::vec3 cam_up = glm::vec3(0, 1, 0);
          glm::vec3 min = glm::vec3(env.world_min);
          glm::vec3 max = glm::vec3(env.world_max);
          glm::vec3 center = {};
          if (frame_render_data->primary_camera != nullptr) {
            cam_up = frame_render_data->primary_camera->up();
            center = frame_render_data->primary_camera->center();
          }
          if (glm::length(glm::cross(to_sun, cam_up)) < 0.001f) {
            cam_up = glm::vec3(0, 0, 1);
            center = (min + max) * 0.5f;
          }

          float radius = 0.5f * glm::length(max - min);
          glm::vec3 eye = center + to_sun * radius;
          glm::mat4 light_view = glm::lookAt(eye, center, cam_up);

          glm::mat4 light_proj = glm::ortho(-radius, radius, -radius, radius, 0.0f, 2.0f * radius);
          glm::mat4 light_space_matrix = light_proj * light_view;

          renderer_ptr->get_resource<shader>(*n.pass->shader_handle)
            .set_uniform("OE_light_space_matrix", light_space_matrix)
            .set_uniform("OE_sun_dir", glm::normalize(env.sun_direction))
            .set_uniform("OE_num_lights", static_cast<int32_t>(frame_render_data->lights.size()))
            .set_uniform("OE_window_size", glm::vec2(renderer_ptr->get_window_size()));
        }

        {
          pass_context ctx{
            renderer_ptr,
            this,
            &n,
            frame_render_data,
            bv,
            &diag,
          };
          exec(ctx);
        }
        n.end_pass(renderer_ptr);
      }

      for (auto& ring : runtime.state.per_draw_rings) {
        ring.head = 0;
      }
    }
  }

  void render_pipeline::reset_draw_buffers() {
    for (auto& [_, runtime] : pass_runtimes) {
      for (auto& ring : runtime.state.per_draw_rings) {
        ring.head = 0;
      }
    }
  }

  glm::ivec2 render_pipeline::get_window_size() const {
    OTHER_ASSERT(renderer_ptr != nullptr, "render_pipeline::get_window_size: no renderer available.");
    return renderer_ptr->get_window_size();
  }

  void render_pipeline::register_texture_resource(const std::string_view name, resource_handle handle) {
    auto hash = FNV(name);
    auto itr = texture_resources.find(hash);
    if (itr == texture_resources.end()) {
      CORE_LOG_ERROR("Texture resource with name '{}' not declared in pipeline '{}'.", name, definition.name);
      return;
    }
    replace_texture_resource(name, handle);
  }

  void render_pipeline::register_buffer_resource(const std::string_view name, resource_handle handle) {
    auto hash = FNV(name);
    auto itr = buffer_resources.find(hash);
    if (itr == buffer_resources.end()) {
      CORE_LOG_ERROR("Buffer resource with name '{}' not declared in pipeline '{}'.", name, definition.name);
      return;
    }

    if (!renderer_ptr->resource_exists(handle)) {
      CORE_LOG_ERROR("Cannot register buffer resource '{}' with handle {} — resource does not exist.", name, handle);
      return;
    }

    // destroy existing resource if handle is changing
    if (itr->second.handle != handle) {
      if (renderer_ptr->resource_exists(itr->second.handle)) {
        renderer_ptr->destroy_resource(itr->second.handle);
      }
      itr->second.handle = handle;
    }
  }

  void render_pipeline::register_shader_resource(const std::string_view name, resource_handle handle) {
    auto hash = FNV(name);
    auto itr = shader_handles.find(hash);
    if (itr == shader_handles.end()) {
      CORE_LOG_ERROR("Shader resource with name '{}' not declared in pipeline '{}'.", name, definition.name);
      return;
    }

    if (!renderer_ptr->resource_exists(handle)) {
      CORE_LOG_ERROR("Cannot register shader resource '{}' with handle {} — resource does not exist.", name, handle);
      return;
    }

    // destroy existing resource if handle is changing
    if (itr->second != handle) {
      if (renderer_ptr->resource_exists(itr->second)) {
        renderer_ptr->destroy_resource(itr->second);
      }
      itr->second = handle;
    }
  }

  ostd::vector<std::string> render_pipeline::get_texture_names() const {
    ostd::vector<std::string> names;
    for (const auto& [_, res] : texture_resources) {
      names.push_back(res.name);
    }
    return names;
  }

  ImTextureID render_pipeline::get_final_output_texture_id() {
    if (!screen_texture_handle.has_value() || !get_renderer()->resource_exists(*screen_texture_handle)) {
      CORE_LOG_ERROR("Screen texture handle not set or resource does not exist for pipeline '{}'.", definition.name);
      return 0;
    }
    return get_renderer()->get_resource<texture>(*screen_texture_handle).get_imgui_texture_id();
  }

  ImTextureID render_pipeline::get_texture_id(const std::string_view name) {
    auto handle_opt = find_texture_by_name(name);
    if (!handle_opt.has_value() || !get_renderer()->resource_exists(*handle_opt)) {
      CORE_LOG_ERROR("Texture resource with name '{}' not found or does not exist for pipeline '{}'.", name, definition.name);
      return 0;
    }
    return get_renderer()->get_resource<texture>(*handle_opt).get_imgui_texture_id();
  }

  glm::ivec2 render_pipeline::get_texture_size(const std::string_view name) const {
    auto handle_opt = find_texture_by_name(name);
    if (!handle_opt.has_value() || !get_renderer()->resource_exists(*handle_opt)) {
      CORE_LOG_ERROR("Texture resource with name '{}' not found or does not exist for pipeline '{}'.", name, definition.name);
      return { 0, 0 };
    }
    return get_renderer()->get_resource<texture>(*handle_opt).get_size();
  }

  opt<resource_handle> render_pipeline::get_final_output_texture() const {
    if (!screen_texture_handle.has_value() || !get_renderer()->resource_exists(*screen_texture_handle)) {
      CORE_LOG_ERROR("Screen texture handle not set or resource does not exist for pipeline '{}'.", definition.name);
      return std::nullopt;
    }
    return get_screen_texture();
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

  void render_pipeline::apply_uniforms(shader& s, const ostd::map<std::string, value>& uniforms) {
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
  namespace {

    constexpr uint32_t align_up(uint32_t value, uint32_t align) {
      OTHER_ASSERT(align > 0 && (align & (align - 1)) == 0, "align_up: alignment must be a power of two, got {}", align);
      return (value + align - 1) & ~(align - 1);
    }

  }  // namespace

  std::string render_pipeline::get_pipeline_name(const std::string_view n) const {
    return
      /// \todo: buffers break with this because the *resource* name and
      ///        the name the buffer resource uses in the shader are coupled
      ///        fix this
      // definition.name + ":" +
      std::string{ n };
  }

  void render_pipeline::build_pass_runtimes() {
    pass_runtimes.clear();

    for (const auto& pass_def : definition.passes) {
      natural_t pass_name_hash = FNV(pass_def.name);
      auto nodes_view = graph->get_graph().nodes | std::views::values;
      render_pass* pass = nullptr;
      {
        auto itr = std::ranges::find_if(nodes_view, [&](const frame_node& node) { return FNV(node.pass->name) == pass_name_hash; });
        if (itr == std::ranges::end(nodes_view)) {
          CORE_LOG_ERROR("No node found in graph for pass '{}'", pass_def.name);
          continue;
        }
        frame_node& n = *itr;
        pass = n.pass;
      }

      OTHER_ASSERT(pass != nullptr, "Node for pass '{}' has null pass pointer", pass_def.name);

      auto [itr, inserted] = pass_runtimes.insert({ pass->id, {} });
      OTHER_ASSERT(inserted, "Pass runtime for pass '{}' already exists", pass_def.name);
      pass_runtime& runtime = itr->second;
      build_pass_runtime(runtime, pass, pass_def);
    }
  }

  void render_pipeline::destroy_pass_runtimes() {
    for (auto& [_, runtime] : pass_runtimes) {
      for (auto& ring : runtime.state.per_draw_rings) {
        if (ring.ring_buffer.id != 0) {
          renderer_ptr->destroy_resource(ring.ring_buffer);
        }
        arena::free(ring.cpu_staging, ring.capacity);
      }
    }
    pass_runtimes.clear();
  }

  void render_pipeline::override_pass_executor(const std::string_view pass_name, executor_fn&& fn) {
    executor_overrides[std::string{ pass_name }] = std::move(fn);
  }

  void render_pipeline::create_resources_from_def() {
    for (const auto& buf_def : definition.buffers) {
      resource_handle handle = gpu_buffer::create(get_pipeline_name(buf_def.name), buf_def.type, buf_def.usage);
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

      uint32_t mips = tex_def.mip_levels;
      if (mips == 0) {
        mips = get_renderer()->rendering()->api()->full_mip_chain_count(size, tex_def.type, tex_def.depth);
      }

      resource_handle handle;
      if (tex_def.type == texture::tex_type::TEXTURE_CUBE) {
        handle = cube_map::create(get_pipeline_name(tex_def.name), tex_def.format, size.x, size.y);
      } else if (tex_def.type == texture::tex_type::TEXTURE_3D) {
        glm::vec3 dimensions(size.x, size.y, tex_def.depth > 0 ? tex_def.depth : 1);
        handle = texture::create3d(get_pipeline_name(tex_def.name), tex_def.format,
                                   tex_def.filters.value_or(std::pair{ texture::LINEAR, texture::LINEAR }),
                                   tex_def.wraps.value_or(std::tuple{ texture::CLAMP_TO_EDGE, texture::CLAMP_TO_EDGE, texture::CLAMP_TO_EDGE }),
                                   mips, tex_def.generate_mips, dimensions);
      } else if (tex_def.filters.has_value() && tex_def.wraps.has_value()) {
        handle = texture::create(get_pipeline_name(tex_def.name), tex_def.type, tex_def.format, *tex_def.filters, *tex_def.wraps,
                                 mips, tex_def.generate_mips, size.x, size.y);
      } else {
        handle = texture::create(get_pipeline_name(tex_def.name), tex_def.type, tex_def.format, size.x, size.y);
      }

      if (tex_def.seed_texture_path.has_value()) {
        ostd::vector<uint8_t> bytes = read_seed_texture_file(tex_def.seed_texture_path.value());
        if (!bytes.empty()) {
          (*get_renderer()->rendering()->api()->get_resource_as<texture>(handle))
            .set_data(bytes.data(), bytes.size())
            .finalize_texture();
        }
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
        handle = shader::create(get_pipeline_name(shader_def.name), shader_def.vertex_path, *shader_def.geometry_path, shader_def.fragment_path, shader_def.defines);
      } else if (shader_def.compute_path.has_value()) {
        handle = shader::create(get_pipeline_name(shader_def.name), *shader_def.compute_path, shader_def.defines);
      } else {
        handle = shader::create(get_pipeline_name(shader_def.name), shader_def.vertex_path, shader_def.fragment_path, shader_def.defines);
      }
      shader_handles[FNV(shader_def.name)] = handle;
      CORE_LOG_DEBUG("Created shader resource [{}] with handle {}.", shader_def.name, handle);
    }

    bool needs_quad = false;
    for (const auto& pass : definition.passes) {
      /// scene_grids draws the quad as world-space grid geometry
      if (pass.executor.name == "fullscreen_quad" || pass.executor.name == "scene_grids") {
        needs_quad = true;
        break;
      }
    }

    if (!needs_quad) {
      return;
    }

    quad_mesh_handle = get_renderer()->create_resource(get_pipeline_name("__pipeline_quad_mesh"), resource_type::MESH);
    get_renderer()
      ->get_resource<mesh>(*quad_mesh_handle)
      .set_primitive_type(mesh::primitive_type::TRIANGLES)
      .add_attribute("position", mesh::attribute_type::FLOAT, 2, 0)
      .add_attribute("tex_coords", mesh::attribute_type::FLOAT, 2, 2)
      .upload_vertex_buffer("quad_vertices", gpu_buffer::usage::STATIC, 6, kQuadVertices, sizeof(kQuadVertices))
      .finalize_mesh();
  }

  void render_pipeline::replace_texture_resource(const std::string_view name, resource_handle new_handle) {
    auto hash = FNV(name);
    auto itr = texture_resources.find(hash);
    if (itr == texture_resources.end()) {
      CORE_LOG_ERROR("Texture resource with name '{}' not declared in pipeline '{}'.", name, definition.name);
      return;
    }

    CORE_LOG_DEBUG("Replacing texture resource [{}] in pipeline '{}' with new handle {}.", name, definition.name, new_handle);
    graph->replace_texture_resource(itr->second.handle, new_handle);
    if (renderer_ptr->resource_exists(itr->second.handle)) {
      renderer_ptr->destroy_resource(itr->second.handle);
    }
    itr->second.handle = new_handle;
  }

  void render_pipeline::replace_buffer_resource(const std::string_view name, resource_handle new_handle) {
    auto hash = FNV(name);
    auto itr = buffer_resources.find(hash);
    if (itr == buffer_resources.end()) {
      CORE_LOG_ERROR("Buffer resource with name '{}' not declared in pipeline '{}'.", name, definition.name);
      return;
    }

    CORE_LOG_DEBUG("Replacing buffer resource '{}' in pipeline '{}' with new handle {}.", name, definition.name, new_handle);
    graph->replace_buffer_resource(itr->second.handle, new_handle);
    if (renderer_ptr->resource_exists(itr->second.handle)) {
      renderer_ptr->destroy_resource(itr->second.handle);
    }
    itr->second.handle = new_handle;
  }

  void render_pipeline::replace_shader_resource(const std::string_view name, resource_handle new_handle) {
    auto hash = FNV(name);
    auto itr = shader_handles.find(hash);
    if (itr == shader_handles.end()) {
      CORE_LOG_ERROR("Shader resource with name '{}' not declared in pipeline '{}'.", name, definition.name);
      return;
    }

    CORE_LOG_DEBUG("Replacing shader resource '{}' in pipeline '{}' with new handle {}.", name, definition.name, new_handle);
    graph->replace_pass_shader(itr->second, new_handle);
    if (renderer_ptr->resource_exists(itr->second)) {
      renderer_ptr->destroy_resource(itr->second);
    }
    itr->second = new_handle;
  }

  void render_pipeline::build_tag_maps() {
    for (const auto& [hash, res] : buffer_resources) {
      if (res.tag != resource_tag::none() && !tagged_buffer_handles.contains(res.tag)) {
        auto itr = tagged_buffer_handles.insert({ res.tag, res.handle });
        if (!itr.second) {
          CORE_LOG_ERROR("Failed to insert tagged buffer handle for tag: {}", res.tag.value());
        }
      }
    }
    for (const auto& [hash, res] : texture_resources) {
      if (res.tag != resource_tag::none() && !tagged_texture_handles.contains(res.tag)) {
        auto itr = tagged_texture_handles.insert({ res.tag, res.handle });
        if (!itr.second) {
          CORE_LOG_ERROR("Failed to insert tagged texture handle for tag: {}", res.tag.value());
        }
      }
    }
  }

  void render_pipeline::build_passes_from_def() {
    for (const auto& pass_def : definition.passes) {
      CORE_LOG_DEBUG("Building render-pass: {}", pass_def.name);
      /// find the shader for this pass
      opt<resource_handle> shader_handle = get_shader_handle(pass_def.shader_name);
      if (!shader_handle.has_value()) {
        CORE_LOG_ERROR("Shader [{}] for pass [{}] not found among pipeline shaders.", pass_def.shader_name, pass_def.name);
        continue;
      }

      glm::ivec2 size = resolve_size(pass_def.use_window_size, pass_def.fixed_size);
      auto builder = graph->start_pass(pass_def.name, shader_handle, pass_def.pass_type, size, pass_def.create_framebuffer);
      build_pass(pass_def, builder);
      builder.pass.clear_flags = pass_def.clear_flags;
      builder.pass.override_fb_clear = pass_def.override_fb_clear;
      builder.end_pass();
    }
  }

  void render_pipeline::validate() {
    for (resource_tag tag : definition.required_tags) {
      if (!tagged_buffer_handles.contains(tag) && !tagged_texture_handles.contains(tag)) {
        CORE_LOG_ERROR("Pipeline [{}] requires tag [{}] but no resource provides it.", definition.name, tag.value());
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
      CORE_LOG_DEBUG(" - setting clear color: {}", *pass_def.clear_color);
      builder.set_clear_color(*pass_def.clear_color);
    }

    for (const auto& ref : pass_def.inputs) {
      const bool is_buffer = is_buffer_resource(ref.resource_name);
      CORE_LOG_DEBUG(" - input resource: {}, type: {}", ref.resource_name, is_buffer ? "buffer" : "texture");
      if (is_buffer) {
        auto handle = find_buffer_by_name(ref.resource_name);
        OTHER_ASSERT(handle.has_value(), "Input resource [{}] for pass [{}] not found as either buffer or texture.", ref.resource_name, pass_def.name);
        builder.buffer_resource(*handle, ref.binding, ref.access);
      } else {
        auto handle = find_texture_by_name(ref.resource_name);
        OTHER_ASSERT(handle.has_value(), "Input resource [{}] for pass [{}] not found as either buffer or texture.", ref.resource_name, pass_def.name);
        builder.texture_resource(*handle, ref.uniform_name, ref.binding, ref.attachment, ref.access, ref.mip_level);
      }
    }

    for (const auto& ref : pass_def.outputs) {
      const bool is_buffer = is_buffer_resource(ref.resource_name);
      CORE_LOG_DEBUG(" - output resource: {}, attachment: {}, type: {}", ref.resource_name, ref.attachment, is_buffer ? "buffer" : "texture");
      if (is_buffer) {
        auto handle = find_buffer_by_name(ref.resource_name);
        OTHER_ASSERT(handle.has_value(), "Output resource [{}] for pass [{}] not found as either buffer or texture.", ref.resource_name, pass_def.name);
        builder.buffer_resource(*handle, ref.binding, ref.access);
      } else {
        auto handle = find_texture_by_name(ref.resource_name);
        OTHER_ASSERT(handle.has_value(), "Output resource [{}] for pass [{}] not found as either buffer or texture.", ref.resource_name, pass_def.name);
        builder.texture_resource(*handle, ref.uniform_name, ref.binding, ref.attachment, ref.access, ref.mip_level);
      }
    }

    for (const auto& [id, uniform] : pass_def.uniforms) {
      builder.add_uniform(uniform.name, uniform.val);
    }

    /// add depends_on tags
    for (const auto& depends_on_str : pass_def.depends_on) {
      builder.depends_on(depends_on_str);
    }

    /// set up executor and check for runtime override
    auto executor = make_executor(pass_def);
    if (executor == nullptr) {
      CORE_LOG_ERROR("Failed to create render pass executor for pass {}! invalid executor: {}", pass_def.name, pass_def.executor.name);
      return;
    }
    CORE_LOG_DEBUG(" - setting up executor: {}", pass_def.executor.name);

    auto override_itr = executor_overrides.find(pass_def.name);
    if (override_itr != executor_overrides.end()) {
      executor = override_itr->second;
    }

    builder.execution_callback(std::move(executor));
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
    if (auto shader_itr = shader_handles.find(FNV(shader_name)); shader_itr != shader_handles.end()) {
      return shader_itr->second;
    }
    return std::nullopt;
  }

  pass_runtime& render_pipeline::build_pass_runtime(pass_runtime& runtime, render_pass* pass, const pipeline_pass_definition& pass_def) {
    OTHER_ASSERT(pass != nullptr, "Cannot build pass runtime for null pass pointer.");

    runtime.pass_id = pass->id;
    runtime.def = &pass_def;
    runtime.state.per_frame_handles.resize(pass_def.bindings.size());
    runtime.state.per_draw_rings.resize(pass_def.bindings.size());

    auto& reg = renderer_ptr->get_binding_registry();
    for (size_t i = 0; i < pass_def.bindings.size(); ++i) {
      const auto& bd = pass_def.bindings[i];

      if (bd.scope == binding_scope::PER_DRAW_CALL || bd.scope == binding_scope::PER_INSTANCE) {
        OTHER_ASSERT(bd.element_size > 0, "pipeline '{}' pass '{}' binding '{}': per-draw bindings must specify element_size > 0", definition.name, pass_def.name, bd.name);

        uint32_t align = 1;
        switch (bd.type) {
          case binding_type::UNIFORM_BUFFER: align = renderer_ptr->rendering()->api()->uniform_buffer_offset_alignment(); break;
          case binding_type::STORAGE_BUFFER: align = renderer_ptr->rendering()->api()->storage_buffer_offset_alignment(); break;
          /// \todo is this correct?
          case binding_type::DRAW_INDIRECT_BUFFER: align = 4; break;
          default:
            OTHER_ASSERT(false, "binding '{}' has type {} but a per-draw ring was requested", bd.name, int(bd.type));
        }

        const uint32_t stride = align_up(bd.element_size, align);
        const uint32_t expected = pass_def.expected_max_draws.value_or(renderer::kMaxDrawCalls);
        const uint32_t capacity = stride * expected;  // * kMaxFramesInFlight;
        // clang-format off
          CORE_LOG_DEBUG("Attempting to allocate per-draw ring buffer for pass '{}' binding '{}': element_size={}, expected_max_draws={}, capacity={}", 
                         pass_def.name, bd.name, bd.element_size, expected, capacity);
        // clang-format on

        auto handle = renderer_ptr->create_resource(get_pipeline_name(std::format("{}.{}.ring", pass_def.name, bd.name)), resource_type::BUFFER);
        runtime.state.per_draw_rings[i] = {
          .ring_buffer = handle,
          .cpu_staging = (uint8_t*)arena::allocate(capacity),
          .capacity = capacity,
          .element_size = bd.element_size,
          .stride = stride,
          .head = 0,
          .binding_point = bd.binding,
          .set = bd.set,
        };

        gpu_buffer& buf = renderer_ptr->get_resource<gpu_buffer>(handle);
        buf.set_buffer_type(buffer_type_from_binding(bd.type))
          .set_usage(gpu_buffer::usage::DYNAMIC)
          .set_data(nullptr, capacity)
          .finalize_buffer();
      }

      bool ok = true;
      switch (bd.scope) {
        case binding_scope::PER_PIPELINE:
          ok = true;
          break;
        case binding_scope::PER_FRAME:
          ok = !bd.tag.is_none() && (reg.find_per_frame(bd.tag) != nullptr || renderer_ptr->frame_binder_resolves(bd.tag));
          break;
        case binding_scope::PER_DRAW_CALL:
          /// the material tag is produced by the pipeline itself when it declares a layout
          ok = !bd.tag.is_none() &&
               ((bd.tag.value() == resource_tag::kMaterialTag && definition.materials.has_value()) ||
                reg.find_per_draw(bd.tag) != nullptr || renderer_ptr->draw_binder_resolves(bd.tag));
          break;
        case binding_scope::PER_INSTANCE:
          ok = !bd.tag.is_none() && (reg.find_per_instance(bd.tag) != nullptr || renderer_ptr->instance_binder_resolves(bd.tag));
          break;
        default:
          OTHER_ASSERT(false, "Unsupported binding scope {} for pass '{}', binding '{}'", bd.scope, pass_def.name, bd.name);
      }

      if (!ok) {
        // clang-format off
          CORE_LOG_ERROR("pipeline '{}' pass '{}': no producer for binding '{}' (scope={}, tag={})",
                         definition.name, pass_def.name, bd.name, bd.scope, bd.tag.value());
        // clang-format on
        valid = false;
      }
    }

    return runtime;
  }

}  // namespace other