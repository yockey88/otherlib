/**
 * \file renderer/pipeline_definition.cpp
 **/
#include "renderer/pipeline_definition.hpp"

#include <toml++/toml.hpp>

#include "renderer/gpu_structs.hpp"
#include "renderer/util/pipeline_asset_validation.hpp"

namespace other {

  executor_type executor_type_from_string(const std::string_view str) {
    natural_t hash = FNV(str);
    switch (hash) {
      case FNV("draw_scene"): return executor_type::DRAW_SCENE;
      case FNV("fullscreen_quad"): return executor_type::FULLSCREEN_QUAD;
      case FNV("compute_dispatch"): return executor_type::COMPUTE_DISPATCH;
      case FNV("window_sized_compute_dispatch"): return executor_type::WINDOW_SIZED_COMPUTE_DISPATCH;
      case FNV("voxelize"): return executor_type::VOXELIZE;
      case FNV("noop"): return executor_type::NOOP;
      case FNV("script"): return executor_type::SCRIPT;
      default: return executor_type::NOOP;
    }
  }

  shader::compute_barrier_type compute_barrier_type_from_string(const std::string_view str) {
    natural_t hash = FNV(str);
    switch (hash) {
      case FNV("shader_image_access"): return shader::compute_barrier_type::SHADER_IMAGE_ACCESS;
      case FNV("shader_storage"): return shader::compute_barrier_type::SHADER_STORAGE;
      case FNV("uniform"): return shader::compute_barrier_type::UNIFORM_BARRIER;
      case FNV("texture_fetch"): return shader::compute_barrier_type::TEXTURE_FETCH;
      case FNV("all"): return shader::compute_barrier_type::ALL_BARRIER;
      case FNV("none"): [[fallthrough]];
      default: return shader::compute_barrier_type::NONE;
    }
  }

  std::string_view executor_type_to_string(executor_type type) {
    switch (type) {
      case executor_type::DRAW_SCENE: return "draw_scene";
      case executor_type::FULLSCREEN_QUAD: return "fullscreen_quad";
      case executor_type::COMPUTE_DISPATCH: return "compute_dispatch";
      case executor_type::WINDOW_SIZED_COMPUTE_DISPATCH: return "window_sized_compute_dispatch";
      case executor_type::VOXELIZE: return "voxelize";
      case executor_type::NOOP: return "noop";
      case executor_type::SCRIPT: return "script";
      default: return "noop";
    }
  }

  gpu_buffer::buf_type buffer_type_from_binding(binding_type type) {
    switch (type) {
      case binding_type::UNIFORM_BUFFER: return gpu_buffer::buf_type::UNIFORM_BUFFER;
      case binding_type::STORAGE_BUFFER: return gpu_buffer::buf_type::STORAGE_BUFFER;
      case binding_type::DRAW_INDIRECT_BUFFER: return gpu_buffer::buf_type::DRAW_INDIRECT_BUFFER;
      default:
        OTHER_ASSERT(false, "Unsupported binding type {} for buffer resource", type);
        return gpu_buffer::buf_type::UNIFORM_BUFFER;
    }
  }

  gpu_buffer::buf_type buffer_type_from_string(const std::string_view str) {
    switch (FNV(str)) {
      case FNV("UNIFORM_BUFFER"): return gpu_buffer::buf_type::UNIFORM_BUFFER;
      case FNV("STORAGE_BUFFER"): return gpu_buffer::buf_type::STORAGE_BUFFER;
      case FNV("DRAW_INDIRECT_BUFFER"): return gpu_buffer::buf_type::DRAW_INDIRECT_BUFFER;
      default:
        OTHER_ASSERT(false, "Unsupported buffer type string {}", str);
        return gpu_buffer::buf_type::UNIFORM_BUFFER;
    }
  }

  gpu_buffer::usage buffer_usage_from_string(const std::string_view str) {
    switch (FNV(str)) {
      case FNV("STATIC"): return gpu_buffer::usage::STATIC;
      case FNV("DYNAMIC"): return gpu_buffer::usage::DYNAMIC;
      case FNV("STREAM"): return gpu_buffer::usage::STREAM;
      default:
        OTHER_ASSERT(false, "Unsupported buffer usage string {}", str);
        return gpu_buffer::usage::DYNAMIC;
    }
  }

  texture::tex_type texture_type_from_string(const std::string_view str) {
    switch (FNV(str)) {
      case FNV("TEXTURE_1D"): return texture::tex_type::TEXTURE_1D;
      case FNV("TEXTURE_2D"): return texture::tex_type::TEXTURE_2D;
      case FNV("TEXTURE_3D"): return texture::tex_type::TEXTURE_3D;
      case FNV("TEXTURE_CUBE"): return texture::tex_type::TEXTURE_CUBE;
      case FNV("TEXTURE_CUBE_FACE_POSITIVE_X"): return texture::tex_type::TEXTURE_CUBE_FACE_POSITIVE_X;
      case FNV("TEXTURE_CUBE_FACE_NEGATIVE_X"): return texture::tex_type::TEXTURE_CUBE_FACE_NEGATIVE_X;
      case FNV("TEXTURE_CUBE_FACE_POSITIVE_Y"): return texture::tex_type::TEXTURE_CUBE_FACE_POSITIVE_Y;
      case FNV("TEXTURE_CUBE_FACE_NEGATIVE_Y"): return texture::tex_type::TEXTURE_CUBE_FACE_NEGATIVE_Y;
      case FNV("TEXTURE_CUBE_FACE_POSITIVE_Z"): return texture::tex_type::TEXTURE_CUBE_FACE_POSITIVE_Z;
      case FNV("TEXTURE_CUBE_FACE_NEGATIVE_Z"): return texture::tex_type::TEXTURE_CUBE_FACE_NEGATIVE_Z;
      default:
        OTHER_ASSERT(false, "Unsupported texture type string {}", str);
        return texture::tex_type::TEXTURE_2D;
    }
  }

  texture::format texture_format_from_string(const std::string_view str) {
    switch (FNV(str)) {
      case FNV("R1"): return texture::format::R1;
      case FNV("A8"): return texture::format::A8;
      case FNV("R8"): return texture::format::R8;
      case FNV("R8I"): return texture::format::R8I;
      case FNV("R8U"): return texture::format::R8U;
      case FNV("R8S"): return texture::format::R8S;
      case FNV("R16"): return texture::format::R16;
      case FNV("R16I"): return texture::format::R16I;
      case FNV("R16U"): return texture::format::R16U;
      case FNV("R16F"): return texture::format::R16F;
      case FNV("R16S"): return texture::format::R16S;
      case FNV("R32I"): return texture::format::R32I;
      case FNV("R32U"): return texture::format::R32U;
      case FNV("R32F"): return texture::format::R32F;
      case FNV("RG8"): return texture::format::RG8;
      case FNV("RG8I"): return texture::format::RG8I;
      case FNV("RG8U"): return texture::format::RG8U;
      case FNV("RG8S"): return texture::format::RG8S;
      case FNV("RG16"): return texture::format::RG16;
      case FNV("RG16I"): return texture::format::RG16I;
      case FNV("RG16U"): return texture::format::RG16U;
      case FNV("RG16F"): return texture::format::RG16F;
      case FNV("RG16S"): return texture::format::RG16S;
      case FNV("RG32I"): return texture::format::RG32I;
      case FNV("RG32U"): return texture::format::RG32U;
      case FNV("RG32F"): return texture::format::RG32F;
      case FNV("RGB8"): return texture::format::RGB8;
      case FNV("RGB8I"): return texture::format::RGB8I;
      case FNV("RGB8U"): return texture::format::RGB8U;
      case FNV("RGB8S"): return texture::format::RGB8S;
      case FNV("RGB9E5F"): return texture::format::RGB9E5F;
      case FNV("BGRA8"): return texture::format::BGRA8;
      case FNV("RGBA8"): return texture::format::RGBA8;
      case FNV("RGBA8I"): return texture::format::RGBA8I;
      case FNV("RGBA8U"): return texture::format::RGBA8U;
      case FNV("RGBA8S"): return texture::format::RGBA8S;
      case FNV("RGBA16"): return texture::format::RGBA16;
      case FNV("RGBA16I"): return texture::format::RGBA16I;
      case FNV("RGBA16U"): return texture::format::RGBA16U;
      case FNV("RGBA16F"): return texture::format::RGBA16F;
      case FNV("RGBA16S"): return texture::format::RGBA16S;
      case FNV("RGBA32I"): return texture::format::RGBA32I;
      case FNV("RGBA32U"): return texture::format::RGBA32U;
      case FNV("RGBA32F"): return texture::format::RGBA32F;
      case FNV("B5G6R5"): return texture::format::B5G6R5;
      case FNV("R5G6B5"): return texture::format::R5G6B5;
      case FNV("BGRA4"): return texture::format::BGRA4;
      case FNV("RGBA4"): return texture::format::RGBA4;
      case FNV("BGR5A1"): return texture::format::BGR5A1;
      case FNV("RGB5A1"): return texture::format::RGB5A1;
      case FNV("RGB10A2"): return texture::format::RGB10A2;
      case FNV("RG11B10F"): return texture::format::RG11B10F;
      case FNV("DEPTHF"): return texture::format::DEPTHF;
      default:
        OTHER_ASSERT(false, "Unsupported texture format string {}", str);
        return texture::format::RGBA16F;
    }
  }

  render_pass::type render_pass_type_from_string(const std::string_view str) {
    switch (FNV(str)) {
      case FNV("RENDER_PASS"): return render_pass::RENDER_PASS;
      case FNV("COMPUTE_PASS"): return render_pass::COMPUTE_PASS;
      default:
        OTHER_ASSERT(false, "Unsupported render pass type string {}", str);
        return render_pass::RENDER_PASS;
    }
  }

  binding_scope pass_binding_scope_from_string(const std::string_view str) {
    switch (FNV(str)) {
      case FNV("PER_PIPELINE"): return binding_scope::PER_PIPELINE;
      case FNV("PER_FRAME"): return binding_scope::PER_FRAME;
      case FNV("PER_PASS"): return binding_scope::PER_PASS;
      case FNV("PER_DRAW"): return binding_scope::PER_DRAW_CALL;
      case FNV("PER_INSTANCER"): return binding_scope::PER_INSTANCE;
      default:
        OTHER_ASSERT(false, "Unsupported binding scope string {}", str);
        return binding_scope::PER_PASS;
    }
  }

  binding_type pass_binding_type_from_string(const std::string_view str) {
    switch (FNV(str)) {
      case FNV("UNIFORM_BUFFER"): return binding_type::UNIFORM_BUFFER;
      case FNV("STORAGE_BUFFER"): return binding_type::STORAGE_BUFFER;
      case FNV("STORAGE_IMAGE"): return binding_type::STORAGE_IMAGE;
      case FNV("TEXTURE_2D"): return binding_type::TEXTURE_2D;
      case FNV("TEXTURE_ARRAY"): return binding_type::TEXTURE_ARRAY;
      case FNV("DRAW_INDIRECT_BUFFER"): return binding_type::DRAW_INDIRECT_BUFFER;
      default:
        OTHER_ASSERT(false, "Unsupported binding type string {}", str);
        return binding_type::UNIFORM_BUFFER;
    }
  }

  framebuffer::attachment_type framebuffer_attachment_type_from_string(const std::string_view str) {
    switch (FNV(str)) {
      case FNV("DEPTH"): return framebuffer::DEPTH;
      case FNV("STENCIL"): return framebuffer::STENCIL;
      case FNV("DEPTH_STENCIL"): return framebuffer::DEPTH_STENCIL;
      case FNV("COLOR"): return framebuffer::COLOR;
      default:
        OTHER_ASSERT(false, "Unsupported framebuffer attachment type string {}", str);
        return framebuffer::COLOR;
    }
  }

  resource_tag resource_tag_from_string(const std::string_view str) {
    return resource_tag(FNV(str));
  }

  struct frame_binding_table {
    std::string name;
    std::string tag;
    std::string scope;
    std::string type;
    opt<uint32_t> binding = std::nullopt;
    int64_t element_size = 0;
  };
  struct frame_input_output_table {
    std::string pass_name;
    std::string resource_name;
    std::string attachment;
    opt<uint32_t> binding = std::nullopt;
    uint32_t mip_level = 0;
  };
  struct frame_executor_table {
    std::string pass_name;
    std::string name;
    struct uniform_or_param {
      std::string name;
      value val;
    };
    std::vector<uniform_or_param> uniforms;
    std::vector<uniform_or_param> params;
  };
  struct frame_resource_tag {
    std::string name;
    std::string type;
  };
  struct frame_section {
    std::vector<frame_binding_table> bindings;
    std::vector<frame_input_output_table> inputs;
    std::vector<frame_input_output_table> outputs;
    std::vector<frame_executor_table> executors;
    std::vector<frame_resource_tag> resource_tags;
  };

  namespace detail {

    void parse_resources(pipeline_definition& into_def, const toml::table& pipeline_table);
    bool collect_frame_details(pipeline_definition& into_def, frame_section& into_section, const toml::table& pipeline_table);
    void build_pass_definitions(pipeline_definition& into_def, const frame_section& frame, const toml::table& pipeline_table);

  }  // namespace detail

  pipeline_definition read_pipeline_definition_from_file(const filepath& pl_def_path) {
    OTHER_ASSERT(std::filesystem::exists(pl_def_path), "Rendering pipeline asset does not exist! {}", pl_def_path.string());
    OTHER_ASSERT(pl_def_path.extension() == ".toml", "Rendering pipeline asset must be a TOML file! {}", pl_def_path.string());

    toml::table pipeline_table;
    try {
      std::string contents;
      std::stringstream ss;
      std::ifstream file(pl_def_path);
      if (!file.is_open()) {
        CORE_LOG_ERROR("Failed to open rendering pipeline TOML: {}", pl_def_path.string());
        return {};
      }

      ss << file.rdbuf();
      contents = ss.str();
      pipeline_table = toml::parse(contents);
    } catch (const toml::parse_error& e) {
      CORE_LOG_ERROR("Failed to parse rendering pipeline TOML: {}", e.what());
      return {};
    } catch (const std::exception& e) {
      CORE_LOG_ERROR("Failed to parse rendering pipeline TOML: {}", e.what());
      return {};
    } catch (...) {
      CORE_LOG_ERROR("Failed to parse rendering pipeline TOML: unknown error");
      return {};
    }

    OTHER_ASSERT(pipeline_table.contains("asset-type"), "Rendering pipeline TOML must contain an asset-type field");
    std::string asset_type = pipeline_table["asset-type"].as_string()->get();
    OTHER_ASSERT(asset_type == "rendering-pipeline", "Rendering pipeline TOML must have an asset-type of 'rendering-pipeline'");

    CORE_LOG_DEBUG("Parsing rendering pipeline: {}", pl_def_path.string());
    auto validated = detail::validate_pipeline_toml(pipeline_table);
    if (!validated.valid) {
      CORE_LOG_ERROR("Rendering pipeline TOML validation failed");
      for (const auto& error : validated.errors) {
        CORE_LOG_ERROR(" - {}", error);
      }
      return {};
    }

    // since we validated it we should be free to assert almost everywhere

    pipeline_definition definition;
    auto name = pipeline_table.at_path("name");
    auto version = pipeline_table.at_path("version");
    if (!name || !version) {
      OTHER_ASSERT(false, "Rendering pipeline must contain name and version fields, name exists: {}? version exists: {}?", (bool)name, (bool)version);
    }
    if (!name.is_string() || !version.is_number()) {
      OTHER_ASSERT(false, "Rendering pipeline must contain a name field and a version field with correct types, name type: {}? version type: {}?", name.type(), version.type());
    }

    std::string name_str = name.as_string()->get();
    definition.version = version.as_integer()->get();
    if (name_str.empty()) {
      OTHER_ASSERT(false, "Rendering pipeline name cannot be empty");
    }
    CORE_LOG_DEBUG(" - pipeline: {}, version: {}", name_str, definition.version);

    auto shadow_map_pass = pipeline_table.at_path("lighting.shadow_map_pass_name");
    auto shading_pass = pipeline_table.at_path("lighting.shading_pass_name");
    auto light_space_matrix_uniform = pipeline_table.at_path("lighting.light_space_matrix_uniform_name");
    if (shadow_map_pass && shadow_map_pass.is_string()) {
      definition.shadow_map_pass_name = shadow_map_pass.as_string()->get();
    }
    if (shading_pass && shading_pass.is_string()) {
      definition.shading_pass_name = shading_pass.as_string()->get();
    }
    if (light_space_matrix_uniform && light_space_matrix_uniform.is_string()) {
      definition.light_space_matrix_uniform_name = light_space_matrix_uniform.as_string()->get();
    }

    detail::parse_resources(definition, pipeline_table);

    frame_section frame;
    bool valid = detail::collect_frame_details(definition, frame, pipeline_table);
    if (!valid) {
      OTHER_ASSERT(false, "Invalid frame section details");
    }

    CORE_LOG_DEBUG(" - construting passes");
    detail::build_pass_definitions(definition, frame, pipeline_table);

    definition.name = name_str;
    return definition;
  }

  pipeline_definition get_empty_pipeline() {
    pipeline_definition def;
    def.name = "empty";
    def.version = 1;
    /// no passes, no textures, no shaders
    ///     useful for UI-only
    return def;
  }

  namespace detail {

    void parse_resources(pipeline_definition& into_def, const toml::table& pipeline_table) {
      /// resources
      auto buffers = pipeline_table.at_path("resources.buffers");
      auto textures = pipeline_table.at_path("resources.textures");
      auto shaders = pipeline_table.at_path("resources.shaders");
      if (!buffers.is_array_of_tables() || !textures.is_array_of_tables() || !shaders.is_array_of_tables()) {
        OTHER_ASSERT(false, "Resources must be arrays of tables, buffers: {}, textures: {}, shaders: {}", buffers.type(), textures.type(), shaders.type());
      }

      //  buffers
      for (const auto& buffer : *buffers.as_array()) {
        auto name = buffer.at_path("name");
        auto type = buffer.at_path("type");
        auto usage = buffer.at_path("usage");
        auto tag = buffer.at_path("tag");
        if (!name.is_string() || !type.is_string() || !usage.is_string() || !tag.is_string()) {
          OTHER_ASSERT(false, "Buffer must have name, type, usage, and tag of type string, found types - name: {}, type: {}, usage: {}, tag: {}", name.type(), type.type(), usage.type(), tag.type());
        }

        auto& buf = into_def.buffers.emplace_back() = pipeline_buffer_definition{
          .name = name.as_string()->get(),
          .type = buffer_type_from_string(type.as_string()->get()),
          .usage = buffer_usage_from_string(usage.as_string()->get()),
          .tag = resource_tag_from_string(tag.as_string()->get())
        };
        std::stringstream ss;
        ss << "Pipeline Buffer: " << buf.name << "\n"
           << " - type: " << type.as_string()->get() << ",\n"
           << " - usage: " << usage.as_string()->get() << ",\n"
           << " - tag: " << tag.as_string()->get();
        CORE_LOG_DEBUG("{}", ss.str());
      }

      //  textures
      for (const auto& texture : *textures.as_array()) {
        auto name = texture.at_path("name");
        auto use_window_size = texture.at_path("use_window_size");
        auto type = texture.at_path("type");
        auto format = texture.at_path("format");
        auto tag = texture.at_path("tag");
        auto size = texture.at_path("size");
        auto depth = texture.at_path("depth");
        auto mip_levels = texture.at_path("mip_levels");
        auto generate_mips = texture.at_path("generate_mips");
        auto seed_texture_path = texture.at_path("seed_texture_path");
        if (!name || !use_window_size || !type || !format) {
          CORE_LOG_ERROR("name exists: {}, use_window_size exists: {}, type exists: {}, format exists: {}",
                         (bool)name, (bool)use_window_size, (bool)type, (bool)format);
          OTHER_ASSERT(false, "Texture must have name, use_window_size, type, format, missing one or more required fields");
        }
        if (!name.is_string() || !use_window_size.is_boolean() || !type.is_string() || !format.is_string()) {
          CORE_LOG_ERROR("name type: {}, use_window_size type: {}, type type: {}, format type: {}",
                         name.type(), use_window_size.type(), type.type(), format.type());
          OTHER_ASSERT(false, "Texture fields must be of correct types: name (string), use_window_size (boolean), type (string), format (string)");
        }

        auto& tex = into_def.textures.emplace_back() = pipeline_texture_definition{
          .name = name.as_string()->get(),
          .use_window_size = use_window_size.as_boolean()->get(),
          .type = texture_type_from_string(type.as_string()->get()),
          .format = texture_format_from_string(format.as_string()->get())
        };

        if (tag && tag.is_string()) {
          tex.tag = resource_tag_from_string(tag.as_string()->get());
        }

        if (size && size.is_table()) {
          auto x = size.at_path("x");
          auto y = size.at_path("y");
          if (x && x.is_integer()) {
            tex.fixed_size.x = static_cast<uint32_t>(x.as_integer()->get());
          } else {
            CORE_LOG_ERROR("Texture size must have integer fields x and y");
          }
          if (y && y.is_integer()) {
            tex.fixed_size.y = static_cast<uint32_t>(y.as_integer()->get());
          } else {
            CORE_LOG_ERROR("Texture size must have integer fields x and y");
          }
        }
        if (depth && depth.is_integer()) {
          tex.depth = static_cast<uint32_t>(depth.as_integer()->get());
        }
        if (mip_levels && mip_levels.is_integer()) {
          tex.mip_levels = static_cast<uint32_t>(mip_levels.as_integer()->get());
        }
        if (generate_mips && generate_mips.is_boolean()) {
          tex.generate_mips = generate_mips.as_boolean()->get();
        }
        if (seed_texture_path && seed_texture_path.is_string()) {
          tex.seed_texture_path = seed_texture_path.as_string()->get();
          if (!std::filesystem::exists(tex.seed_texture_path.value())) {
            tex.seed_texture_path = std::filesystem::absolute(tex.seed_texture_path.value());
            if (!std::filesystem::exists(tex.seed_texture_path.value())) {
              CORE_LOG_ERROR("Texture 'seed_texture_path' file does not exist: {}", tex.seed_texture_path.value().string());
              tex.seed_texture_path = std::nullopt;
            }
          }
        }

        std::stringstream ss;
        ss << "Pipeline Texture: " << tex.name << "\n"
           << " - use_window_size: " << (tex.use_window_size ? "true" : "false") << ",\n"
           << " - type: " << std::format("{}", tex.type) << ",\n"
           << " - format: " << std::format("{}", tex.format) << ",\n"
           << " - mip_levels: " << tex.mip_levels << ",\n"
           << " - generate_mips: " << (tex.generate_mips ? "true" : "false");
        CORE_LOG_DEBUG("{}", ss.str());
      }

      //  shaders
      for (const auto& shader : *shaders.as_array()) {
        auto name = shader.at_path("name");
        auto compute_path = shader.at_path("compute_path");
        if (compute_path) {
          if (!name.is_string() || !compute_path.is_string()) {
            OTHER_ASSERT(false, "Shader must have name and compute_path of correct types, found types - name: {}, compute_path: {}", name.type(), compute_path.type());
          }
          into_def.shaders.emplace_back() = pipeline_shader_definition{
            .name = name.as_string()->get(),
            .compute_path = compute_path.as_string()->get()
          };
          std::stringstream ss;
          ss << "Pipeline Shader: " << name.as_string()->get() << "\n"
             << " - compute_path: " << compute_path.as_string()->get() << "\n";
          CORE_LOG_DEBUG("{}", ss.str());
        } else {
          auto vertex_path = shader.at_path("vertex_path");
          auto fragment_path = shader.at_path("fragment_path");
          if (!name.is_string() || !vertex_path.is_string() || !fragment_path.is_string()) {
            OTHER_ASSERT(false, "Shader must have name, vertex_path, and fragment_path of correct types, found types - name: {}, vertex_path: {}, fragment_path: {}", name.type(), vertex_path.type(), fragment_path.type());
          }

          auto geometry_path = shader.at_path("geometry_path");

          auto& s = into_def.shaders.emplace_back() = pipeline_shader_definition{
            .name = name.as_string()->get(),
            .vertex_path = vertex_path.as_string()->get(),
            .fragment_path = fragment_path.as_string()->get()
          };

          if (geometry_path && geometry_path.is_string()) {
            s.geometry_path = geometry_path.as_string()->get();
          }
          std::stringstream ss_shader;
          ss_shader << "Pipeline Shader: " << name.as_string()->get() << "\n"
                    << " - vertex_path: " << vertex_path.as_string()->get() << "\n"
                    << " - fragment_path: " << fragment_path.as_string()->get();
          if (s.geometry_path) {
            ss_shader << "\n";
            ss_shader << " - geometry_path: " << s.geometry_path.value();
          }
          CORE_LOG_DEBUG("{}", ss_shader.str());
        }
      }
    }

    bool collect_frame_details(pipeline_definition& into_def, frame_section& into_section, const toml::table& pipeline_table) {
      auto frame_bindings = pipeline_table.at_path("frame.bindings");
      auto frame_inputs = pipeline_table.at_path("frame.inputs");
      auto frame_outputs = pipeline_table.at_path("frame.outputs");
      auto frame_executors = pipeline_table.at_path("frame.executors");
      auto frame_resource_tags = pipeline_table.at_path("frame.resource_tags");
      if (!frame_bindings || !frame_inputs || !frame_outputs || !frame_executors || !frame_resource_tags) {
        CORE_LOG_ERROR("frame_bindings: {}, frame_inputs: {}, frame_outputs: {}, frame_executors: {}, frame_resource_tags: {}",
                       (bool)frame_bindings, (bool)frame_inputs, (bool)frame_outputs, (bool)frame_executors, (bool)frame_resource_tags);
        OTHER_ASSERT(false, "Frame section of the pipeline definition is invalid: all frame subsections must be present");
      }
      if (!frame_bindings.is_array_of_tables() || !frame_inputs.is_array_of_tables() || !frame_outputs.is_array_of_tables() ||
          !frame_executors.is_array_of_tables() || !frame_resource_tags.is_array_of_tables()) {
        CORE_LOG_ERROR("frame_bindings type: {}, frame_inputs type: {}, frame_outputs type: {}, frame_executors type: {}, frame_resource_tags type: {}",
                       frame_bindings.type(), frame_inputs.type(), frame_outputs.type(), frame_executors.type(), frame_resource_tags.type());
        OTHER_ASSERT(false, "Frame section of the pipeline definition is invalid: all frame subsections must be arrays of tables");
      }

      for (const auto& binding : *frame_bindings.as_array()) {
        auto name = binding.at_path("name");
        auto tag = binding.at_path("tag");
        auto scope = binding.at_path("scope");
        auto type = binding.at_path("type");
        auto binding_value = binding.at_path("binding");
        auto element_size = binding.at_path("element_size");
        if (!name || !tag || !scope || !type || !element_size) {
          CORE_LOG_ERROR("name: {}, tag: {}, scope: {}, type: {}, element_size: {}",
                         (bool)name, (bool)tag, (bool)scope, (bool)type, (bool)element_size);
          OTHER_ASSERT(false, "Frame binding is invalid: all fields (name, tag, scope, type, element_size) must be present");
        }
        if (!name.is_string() || !tag.is_string() || !scope.is_string() || !type.is_string() || !element_size.is_number()) {
          CORE_LOG_ERROR("name type: {}, tag type: {}, scope type: {}, type type: {}, element_size type: {}",
                         name.type(), tag.type(), scope.type(), type.type(), element_size.type());
          OTHER_ASSERT(false, "Frame binding is invalid: all fields (name, tag, scope, type, element_size) must be of correct types");
        }

        auto& b = into_section.bindings.emplace_back() = frame_binding_table{
          .name = name.as_string()->get(),
          .tag = tag.as_string()->get(),
          .scope = scope.as_string()->get(),
          .type = type.as_string()->get(),
          .element_size = element_size.as_integer()->get(),
        };
        if (binding_value && binding_value.is_number()) {
          b.binding = binding_value.as_integer()->get();
        }
        std::stringstream ss_binding;
        ss_binding << "Frame Binding: " << b.name << "\n"
                   << " - tag: " << b.tag << ",\n"
                   << " - scope: " << b.scope << ",\n"
                   << " - type: " << b.type << ",\n"
                   << " - element_size: " << b.element_size << ",\n"
                   << " - binding: " << (b.binding.has_value() ? std::to_string(b.binding.value()) : "none");
        CORE_LOG_DEBUG("{}", ss_binding.str());
      }

      auto parse_input_output = [](const std::string_view io_type,
                                   toml::node_view<const toml::node> pass_name, toml::node_view<const toml::node> resource_name,
                                   toml::node_view<const toml::node> attachment, toml::node_view<const toml::node> binding,
                                   toml::node_view<const toml::node> mip_level) -> frame_input_output_table {
        if (!pass_name || !resource_name) {
          CORE_LOG_ERROR("pass-name: {}, resource-name: {}", (bool)pass_name, (bool)resource_name);
          OTHER_ASSERT(false, "Invalid frame {}: pass_name or resource_name is missing", io_type);
        }
        if (!pass_name || !resource_name) {
          CORE_LOG_ERROR("pass-name: {}, resource-name: {}", (bool)pass_name, (bool)resource_name);
          OTHER_ASSERT(false, "Invalid frame {}: pass_name or resource_name is missing", io_type);
        }
        auto io = frame_input_output_table{
          .resource_name = resource_name.as_string()->get(),
        };

        if (attachment) {
          if (!attachment.is_string()) {
            CORE_LOG_ERROR("attachment: {}", attachment.type());
            OTHER_ASSERT(false, "Invalid frame {}: attachment is not a string", io_type);
          }
          io.attachment = attachment.as_string()->get();
        }
        if (binding) {
          if (!binding.is_number()) {
            CORE_LOG_ERROR("binding: {}", binding.type());
            OTHER_ASSERT(false, "Invalid frame {}: binding is not a number", io_type);
          }
          io.binding = binding.as_integer()->get();
        }
        if (mip_level) {
          if (!mip_level.is_integer()) {
            CORE_LOG_ERROR("mip_level: {}", mip_level.type());
            OTHER_ASSERT(false, "Invalid frame {}: mip_level is not an integer", io_type);
          }
          io.mip_level = mip_level.as_integer()->get();
        } else {
          io.mip_level = 0;
        }

        io.pass_name = pass_name.as_string()->get();
        return io;
      };

      for (const auto& inputs : *frame_inputs.as_array()) {
        auto pass_name = inputs.at_path("pass_name");
        auto resource_name = inputs.at_path("resource_name");
        auto attachment = inputs.at_path("attachment");
        auto binding = inputs.at_path("binding");
        auto mip_level = inputs.at_path("mip_level");
        auto io = parse_input_output("input", pass_name, resource_name, attachment, binding, mip_level);
        if (!io.pass_name.empty()) {
          into_section.inputs.push_back(std::move(io));
          std::stringstream ss_input;
          ss_input << "Frame Input: " << io.pass_name << "\n"
                   << " - resource_name: " << io.resource_name << ",\n"
                   << " - attachment: " << io.attachment << ",\n"
                   << " - binding: " << (io.binding.has_value() ? std::to_string(io.binding.value()) : "none") << ",\n"
                   << " - mip_level: " << io.mip_level;
          CORE_LOG_DEBUG("{}", ss_input.str());
        } else {
          CORE_LOG_ERROR("Invalid frame input: pass_name is empty");
        }
      }
      for (const auto& output : *frame_outputs.as_array()) {
        auto pass_name = output.at_path("pass_name");
        auto resource_name = output.at_path("resource_name");
        auto attachment = output.at_path("attachment");
        auto binding = output.at_path("binding");
        auto mip_level = output.at_path("mip_level");
        auto io = parse_input_output("output", pass_name, resource_name, attachment, binding, mip_level);
        if (!io.pass_name.empty()) {
          into_section.outputs.push_back(std::move(io));
          std::stringstream ss_output;
          ss_output << "Frame Output: " << io.pass_name << "\n"
                    << " - resource_name: " << io.resource_name << ",\n"
                    << " - attachment: " << io.attachment << ",\n"
                    << " - binding: " << (io.binding.has_value() ? std::to_string(io.binding.value()) : "none") << ",\n"
                    << " - mip_level: " << io.mip_level;
          CORE_LOG_DEBUG("{}", ss_output.str());
        } else {
          CORE_LOG_ERROR("Invalid frame output: pass_name is empty");
        }
      }
      for (const auto& executor : *frame_executors.as_array()) {
        auto pass_name = executor.at_path("pass_name");
        auto name = executor.at_path("name");
        auto uniforms = executor.at_path("uniforms");
        auto params = executor.at_path("params");
        if (!pass_name || !name) {
          CORE_LOG_ERROR("pass name: {}, name: {}", (bool)pass_name, (bool)name);
          OTHER_ASSERT(false, "Executor missing pass_name or name");
        }
        if (!pass_name.is_string() || !name.is_string()) {
          CORE_LOG_ERROR("Executor pass_name or name is not a string");
          OTHER_ASSERT(false, "Executor pass_name or name is not a string");
        }
        auto& exec = into_section.executors.emplace_back() = frame_executor_table{
          .pass_name = pass_name.as_string()->get(),
          .name = name.as_string()->get(),
        };

        if (uniforms && uniforms.is_array_of_tables()) {
          for (const auto& uniform : *uniforms.as_array()) {
            auto uname = uniform.at_path("name");
            auto value = uniform.at_path("value");
            if (!uname || !value) {
              CORE_LOG_ERROR("Uniform missing name or value");
              CORE_LOG_ERROR("uname exists: {}, value exists: {}", (bool)uname, (bool)value);
              return false;
            }
            if (!uname.is_string() || !(value.is_number() || value.is_floating_point() || value.is_boolean())) {
              CORE_LOG_ERROR("Uniform has invalid name or value type");
              CORE_LOG_ERROR("uname type: {}, value type: {}", uname.type(), value.type());
              return false;
            }

            std::string uname_str = uname.as_string()->get();
            auto& u = exec.uniforms.emplace_back() = frame_executor_table::uniform_or_param{
              .name = uname_str,
            };

            if (value.is_floating_point()) {
              u.val = static_cast<float>(value.as_floating_point()->get());
            } else if (value.is_number()) {
              u.val = static_cast<int32_t>(value.as_integer()->get());
            } else if (value.is_boolean()) {
              u.val = value.as_boolean()->get() ? 1.f : 0.f;
            }
            std::stringstream ss_uniform;
            ss_uniform << "Uniform: " << uname_str << "\n"
                       << " - value: " << u.val.to_string();
            CORE_LOG_DEBUG("{}", ss_uniform.str());
          }
        }

        if (params && params.is_array_of_tables()) {
          for (const auto& param : *params.as_array()) {
            auto name = param.at_path("name");
            if (!name) {
              CORE_LOG_ERROR("Invalid parameter: missing name");
              return false;
            }
            if (!name.is_string()) {
              CORE_LOG_ERROR("Invalid parameter: name must be a string");
              CORE_LOG_ERROR("name type: {}", name.type());
              return false;
            }
            std::string name_str = name.as_string()->get();

            auto value = param.at_path("value");
            if (!value) {
              CORE_LOG_ERROR("Invalid parameter: missing value for '{}'", name_str);
              return false;
            }

            switch (FNV(name_str)) {
              case FNV("barrier"): {
                if (!value.is_string()) {
                  OTHER_ASSERT(false, "Invalid parameter: 'barrier' value must be a string for '{}'", name_str);
                }
                std::string barrier_value = value.as_string()->get();
                exec.params.emplace_back() = frame_executor_table::uniform_or_param{
                  .name = name_str,
                  .val = barrier_value
                };
              } break;
              case FNV("groups"): {
                if (!value.is_array()) {
                  OTHER_ASSERT(false, "Invalid parameter: 'groups' value must be a table for '{}'", name_str);
                }
                glm::vec3 groups = glm::vec3(1, 1, 1);
                auto val = value.as_array();
                for (size_t i = 0; i < val->size(); ++i) {
                  auto& item = val->at(i);
                  if (!item.is_number()) {
                    OTHER_ASSERT(false, "Invalid parameter: 'groups' array elements must be integers for '{}'", name_str);
                  }
                  float final_val = 1.f;
                  if (item.is_floating_point()) {
                    final_val = static_cast<float>(item.as_floating_point()->get());
                    final_val = std::floor(final_val);
                  } else {
                    final_val = static_cast<float>(item.as_integer()->get());
                  }
                  switch (i) {
                    case 0: groups.x = final_val; break;
                    case 1: groups.y = final_val; break;
                    case 2: groups.z = final_val; break;
                    default:
                      OTHER_ASSERT(false, "Invalid parameter: 'groups' array contains more than 3 elements for '{}'", name_str);
                  }
                }

                exec.params.emplace_back() = frame_executor_table::uniform_or_param{
                  .name = name_str,
                  .val = groups
                };

              } break;
              case FNV("voxel_dim"): {
                if (!value.is_integer()) {
                  OTHER_ASSERT(false, "Invalid parameter: 'voxel_dim' must be an integer");
                }
                int voxel_dim = static_cast<int>(value.as_integer()->get());
                exec.params.emplace_back() = frame_executor_table::uniform_or_param{
                  .name = name_str,
                  .val = voxel_dim
                };
              } break;
              default:
                OTHER_ASSERT(false, "Invalid parameter name: '{}'", name_str);
            }
          }

          auto& param = exec.params.back();
          std::stringstream ss_param;
          ss_param << "Parameter: " << param.name << "\n"
                   << " - value: " << param.val.to_string() << "\n";
          CORE_LOG_DEBUG("{}", ss_param.str());
        }
      }

      for (const auto& resource_tag : *frame_resource_tags.as_array()) {
        auto name = resource_tag.at_path("name");
        auto type = resource_tag.at_path("type");
        if (!name || !type) {
          CORE_LOG_ERROR("Invalid resource tag: missing name or type");
          CORE_LOG_ERROR("name exists: {}, type exists: {}", (bool)name, (bool)type);
          OTHER_ASSERT(false, "Invalid resource tag: missing name or type");
        }
        if (!name.is_string() || !type.is_string()) {
          CORE_LOG_ERROR("Invalid resource tag: name and type must be strings");
          CORE_LOG_ERROR("name type: {}, type type: {}", name.type(), type.type());
          OTHER_ASSERT(false, "Invalid resource tag: name and type must be strings");
        }

        std::string name_str = name.as_string()->get();
        std::string type_str = type.as_string()->get();
        into_section.resource_tags.emplace_back() = frame_resource_tag{
          .name = name_str,
          .type = type_str,
        };
        into_def.required_tags.push_back(resource_tag_from_string(name_str));
        std::stringstream ss_resource_tag;
        ss_resource_tag << "Resource Tag: " << name_str << "\n"
                        << " - type: " << type_str << "\n";
        CORE_LOG_DEBUG("{}", ss_resource_tag.str());
      }

      return true;
    }

    void build_pass_definitions(pipeline_definition& into_def, const frame_section& frame, const toml::table& pipeline_table) {
      auto frame_passes = pipeline_table.at_path("frame.passes");
      if (!frame_passes) {
        CORE_LOG_ERROR("frame passes: {}", (bool)frame_passes);
        OTHER_ASSERT(false, "Invalid pipeline table: missing frame.passes");
      }
      if (!frame_passes.is_array_of_tables()) {
        CORE_LOG_ERROR("frame passes type: {}", frame_passes.type());
        OTHER_ASSERT(false, "Invalid pipeline table: frame.passes must be an array");
      }

      for (const auto& pass : *frame_passes.as_array()) {
        auto name = pass.at_path("name");
        auto pass_type = pass.at_path("pass_type");
        auto shader_name = pass.at_path("shader_name");
        if (!name || !pass_type || !shader_name) {
          CORE_LOG_ERROR("name exists: {}, type exists: {}, shader_name exists: {}", (bool)name, (bool)pass_type, (bool)shader_name);
          OTHER_ASSERT(false, "Invalid frame pass: missing name, type or shader_name");
        }
        if (!name.is_string() || !pass_type.is_string() || !shader_name.is_string()) {
          CORE_LOG_ERROR("name type: {}, type type: {}, shader_name type: {}", name.type(), pass_type.type(), shader_name.type());
          OTHER_ASSERT(false, "Invalid frame pass: name, type and shader_name must be strings");
        }

        auto& p = into_def.passes.emplace_back();
        p.name = name.as_string()->get();
        p.pass_type = render_pass_type_from_string(pass_type.as_string()->get());
        p.shader_name = shader_name.as_string()->get();

        auto samples = pass.at_path("samples");
        auto multisample = pass.at_path("multisample");
        if (samples && samples.is_integer()) {
          p.samples = static_cast<uint32_t>(samples.as_integer()->get());
        } else if (multisample && multisample.is_number()) {
          p.samples = static_cast<uint32_t>(multisample.as_integer()->get());
        } else if (multisample && samples && multisample.is_boolean() && samples.is_integer()) {
          if (multisample.as_boolean()->get()) {
            p.samples = static_cast<uint32_t>(samples.as_integer()->get());
          }
        }

        auto clear_color = pass.at_path("clear_color");
        auto create_framebuffer = pass.at_path("create_framebuffer");

        if (clear_color && clear_color.is_array()) {
          auto clear_color_array = clear_color.as_array();
          if (clear_color_array->size() < 3 || clear_color_array->size() > 4) {
            CORE_LOG_ERROR("Invalid clear_color: must be an array of 3 or 4 elements");
          } else {
            p.clear_color = glm::vec4{ 0.f, 0.f, 0.f, 0.f };
            for (size_t i = 0; i < clear_color_array->size(); ++i) {
              if (!clear_color_array->at(i).is_floating_point()) {
                CORE_LOG_ERROR("Invalid clear_color: all elements must be floating point numbers");
              } else {
                switch (i) {
                  case 0: p.clear_color->r = static_cast<float>(clear_color_array->at(i).as_floating_point()->get()); break;
                  case 1: p.clear_color->g = static_cast<float>(clear_color_array->at(i).as_floating_point()->get()); break;
                  case 2: p.clear_color->b = static_cast<float>(clear_color_array->at(i).as_floating_point()->get()); break;
                  case 3: p.clear_color->a = static_cast<float>(clear_color_array->at(i).as_floating_point()->get()); break;
                }
              }
            }
          }
        }

        if (create_framebuffer && create_framebuffer.is_boolean()) {
          p.create_framebuffer = create_framebuffer.as_boolean()->get();
        }

        if (p.create_framebuffer && p.pass_type == render_pass::COMPUTE_PASS) {
          p.create_framebuffer = false;
        }

        auto depends_on = pass.at_path("depends_on");
        if (depends_on && depends_on.is_array()) {
          for (size_t i = 0; i < depends_on.as_array()->size(); ++i) {
            std::string depends_on_str = depends_on.as_array()->at(i).as_string()->get();
            p.depends_on.push_back(depends_on_str);
          }
        }

        std::stringstream ss_pass;
        ss_pass << "Pass: " << p.name << "\n";
        ss_pass << " - bindings: [";
        for (const auto& b : p.bindings) {
          ss_pass << b.name << ": " << std::format("{}", b.type) << ", ";
        }
        ss_pass << "]\n";
        for (const auto& b : p.bindings) {
          ss_pass << b.name << ": " << std::format("{}", b.type) << "\n";
        }
        ss_pass << " - samples: " << p.samples << "\n";
        if (p.clear_color.has_value()) {
          ss_pass << " - clear_color: (" << p.clear_color->r << ", " << p.clear_color->g << ", " << p.clear_color->b << ", " << p.clear_color->a << "),\n";
        }
        ss_pass << " - create_framebuffer: " << (p.create_framebuffer ? "true" : "false");
        CORE_LOG_DEBUG("{}", ss_pass.str());
      }

      auto frame_pass_bindings = pipeline_table.at_path("frame.pass-bindings");
      if (frame_pass_bindings && frame_pass_bindings.is_array_of_tables()) {
        for (const auto& b : *frame_pass_bindings.as_array()) {
          auto pass_name = b.at_path("pass_name");
          auto name = b.at_path("name");
          auto binding = b.at_path("binding");
          if (!pass_name || !name || !binding) {
            CORE_LOG_ERROR("pass_name exists: {}, name exists: {}, binding exists: {}", (bool)pass_name, (bool)name, (bool)binding);
            OTHER_ASSERT(false, "Invalid binding: missing pass_name, name or binding");
          }
          if (!pass_name.is_string() || !name.is_string() || !binding.is_integer()) {
            CORE_LOG_ERROR("pass_name type: {}, name type: {}, binding type: {}", pass_name.type(), name.type(), binding.type());
            OTHER_ASSERT(false, "Invalid binding: pass_name and name must be strings and binding must be an integer");
          }

          std::string binding_name = name.as_string()->get();
          int32_t binding_index = binding.as_integer()->get();
          auto itr = std::ranges::find(frame.bindings, binding_name, &frame_binding_table::name);
          if (itr == frame.bindings.end()) {
            CORE_LOG_ERROR("Binding not found in frame: {}", binding_name);
            continue;
          }

          std::string pass_name_str = pass_name.as_string()->get();
          auto pass_itr = std::ranges::find(into_def.passes, pass_name_str, &pipeline_pass_definition::name);
          if (pass_itr == into_def.passes.end()) {
            OTHER_ASSERT(false, "Binding '{}' references non-existent pass '{}'", binding_name, pass_name_str);
          }

          pass_itr->bindings.emplace_back() = frame_binding_definition{
            .name = binding_name,
            .tag = resource_tag_from_string(itr->tag),
            .scope = pass_binding_scope_from_string(itr->scope),
            .type = pass_binding_type_from_string(itr->type),
            .binding = static_cast<uint32_t>(binding_index),
            .element_size = static_cast<uint32_t>(itr->element_size),
          };
        }
      } else if (frame_pass_bindings) {
        CORE_LOG_ERROR("Invalid frame pass bindings: must be an array of tables");
      }

      for (const auto& i : frame.inputs) {
        auto pass_itr = std::ranges::find(into_def.passes, i.pass_name, &pipeline_pass_definition::name);
        if (pass_itr == into_def.passes.end()) {
          OTHER_ASSERT(false, "Input '{}' references non-existent pass '{}'", i.resource_name, i.pass_name);
        }
        CORE_LOG_DEBUG("Processing input '{}' for pass '{}'", i.resource_name, i.pass_name);

        auto& pass = *pass_itr;
        auto& in = pass.inputs.emplace_back(pipeline_resource_reference{
          .resource_name = i.resource_name,
          .mip_level = i.mip_level,
        });
        if (i.binding.has_value()) {
          in.binding = i.binding.value();
        }
        if (!i.attachment.empty()) {
          in.attachment = framebuffer_attachment_type_from_string(i.attachment);
        }
      }

      for (const auto& o : frame.outputs) {
        auto pass_itr = std::ranges::find(into_def.passes, o.pass_name, &pipeline_pass_definition::name);
        if (pass_itr == into_def.passes.end()) {
          OTHER_ASSERT(false, "Output '{}' references non-existent pass '{}'", o.resource_name, o.pass_name);
        }
        CORE_LOG_DEBUG("Processing output '{}' for pass '{}'", o.resource_name, o.pass_name);

        auto& pass = *pass_itr;
        auto& out = pass.outputs.emplace_back(pipeline_resource_reference{
          .resource_name = o.resource_name,
          .mip_level = o.mip_level,
        });
        if (o.binding.has_value()) {
          out.binding = o.binding.value();
        }
        if (!o.attachment.empty()) {
          out.attachment = framebuffer_attachment_type_from_string(o.attachment);
        }
      }

      for (const auto& e : frame.executors) {
        auto pass_itr = std::ranges::find(into_def.passes, e.pass_name, &pipeline_pass_definition::name);
        if (pass_itr == into_def.passes.end()) {
          OTHER_ASSERT(false, "Executor '{}' references non-existent pass '{}'", e.name, e.pass_name);
        }
        CORE_LOG_DEBUG("Processing executor '{}' for pass '{}'", e.name, e.pass_name);

        pass_itr->executor = pipeline_executor_definition{
          .name = e.name
        };

        for (const auto& u : e.uniforms) {
          pass_itr->executor.uniforms.insert({ u.name, std::move(u.val) });
        }
        for (const auto& p : e.params) {
          pass_itr->executor.params.insert({ p.name, std::move(p.val) });
        }
      }
    }

  };  // namespace detail
}  // namespace other