/**
 * \file renderer/util/pipeline_asset_validation.cpp
 **/
#include "renderer/util/pipeline_asset_validation.hpp"

#include <set>

/// \todo make this more general and don't re-write this for each type of asset

namespace other {
  namespace detail {
    namespace {

      void validate_resources(validation_result& result, const toml::table& tbl, std::set<std::string>& out_buffer_names, std::set<std::string>& out_texture_names, std::set<std::string>& out_shader_names);
      void validate_frame_bindings(validation_result& result, const toml::table& tbl, std::set<std::string>& out_binding_names);
      void validate_frame_inputs_outputs(validation_result& result, const toml::table& tbl);
      void validate_frame_executors(validation_result& result, const toml::table& tbl);
      void validate_frame_resource_tags(validation_result& result, const toml::table& tbl);
      void validate_frame_passes(validation_result& result, const toml::table& tbl, std::set<std::string>& out_pass_names);
      void validate_frame_pass_bindings(validation_result& result, const toml::table& tbl);
      void validate_cross_references(validation_result& result, const toml::table& tbl, const std::set<std::string>& buffer_names, const std::set<std::string>& texture_names,
                                     const std::set<std::string>& shader_names, const std::set<std::string>& binding_names, const std::set<std::string>& pass_names);

    }  // namespace

    validation_result validate_pipeline_toml(const toml::table& tbl) {
      validation_result result;

      const auto asset_type = tbl.at_path("asset-type");
      if (!asset_type.is_string()) {
        result.fail("'asset-type': must be a string");
      } else if (asset_type.as_string()->get() != "rendering-pipeline") {
        result.fail(std::format("'asset-type': expected 'rendering-pipeline', got '{}'", asset_type.as_string()->get()));
      }

      const auto name = tbl.at_path("name");
      if (!name.is_string()) {
        result.fail("'name': must be a string");
      } else if (name.as_string()->get().empty()) {
        result.fail("'name': must not be empty");
      }

      const auto version = tbl.at_path("version");
      if (!version.is_integer()) {
        result.fail("'version': must be an integer");
      }

      std::set<std::string> buffer_names;
      std::set<std::string> texture_names;
      std::set<std::string> shader_names;
      std::set<std::string> binding_names;
      std::set<std::string> pass_names;

      validate_resources(result, tbl, buffer_names, texture_names, shader_names);
      validate_frame_bindings(result, tbl, binding_names);
      validate_frame_inputs_outputs(result, tbl);
      validate_frame_executors(result, tbl);
      validate_frame_resource_tags(result, tbl);
      validate_frame_passes(result, tbl, pass_names);
      validate_frame_pass_bindings(result, tbl);

      if (result.valid) {
        validate_cross_references(result, tbl, buffer_names, texture_names, shader_names, binding_names, pass_names);
      }

      return result;
    }

    namespace {

      constexpr std::string_view kValidBufferTypes[] = {
        "UNIFORM_BUFFER",
        "STORAGE_BUFFER",
        "DRAW_INDIRECT_BUFFER",
      };

      constexpr std::string_view kValidBufferUsages[] = {
        "STATIC",
        "DYNAMIC",
        "STREAM",
      };

      constexpr std::string_view kValidTextureTypes[] = {
        "TEXTURE_1D",
        "TEXTURE_2D",
        "TEXTURE_3D",
        "TEXTURE_CUBE",
        "TEXTURE_CUBE_FACE_POSITIVE_X",
        "TEXTURE_CUBE_FACE_NEGATIVE_X",
        "TEXTURE_CUBE_FACE_POSITIVE_Y",
        "TEXTURE_CUBE_FACE_NEGATIVE_Y",
        "TEXTURE_CUBE_FACE_POSITIVE_Z",
        "TEXTURE_CUBE_FACE_NEGATIVE_Z",
      };

      constexpr std::string_view kValidTextureFormats[] = {
        "R1",
        "A8",
        "R8",
        "R8I",
        "R8U",
        "R8S",
        "R16",
        "R16I",
        "R16U",
        "R16F",
        "R16S",
        "R32I",
        "R32U",
        "R32F",
        "RG8",
        "RG8I",
        "RG8U",
        "RG8S",
        "RG16",
        "RG16I",
        "RG16U",
        "RG16F",
        "RG16S",
        "RG32I",
        "RG32U",
        "RG32F",
        "RGB8",
        "RGB8I",
        "RGB8U",
        "RGB8S",
        "RGB9E5F",
        "BGRA8",
        "RGBA8",
        "RGBA8I",
        "RGBA8U",
        "RGBA8S",
        "RGBA16",
        "RGBA16I",
        "RGBA16U",
        "RGBA16F",
        "RGBA16S",
        "RGBA32I",
        "RGBA32U",
        "RGBA32F",
        "B5G6R5",
        "R5G6B5",
        "BGRA4",
        "RGBA4",
        "BGR5A1",
        "RGB5A1",
        "RGB10A2",
        "RG11B10F",
        "DEPTHF",
      };

      constexpr std::string_view kValidPassTypes[] = {
        "RENDER_PASS",
        "COMPUTE_PASS",
      };

      constexpr std::string_view kValidBindingScopes[] = {
        "PER_PIPELINE",
        "PER_FRAME",
        "PER_PASS",
        "PER_DRAW",
        "PER_INSTANCER",
      };

      constexpr std::string_view kValidBindingTypes[] = {
        "UNIFORM_BUFFER",
        "STORAGE_BUFFER",
        "STORAGE_IMAGE",
        "TEXTURE_2D",
        "TEXTURE_ARRAY",
        "DRAW_INDIRECT_BUFFER",
      };

      constexpr std::string_view kValidAttachmentTypes[] = {
        "DEPTH",
        "STENCIL",
        "DEPTH_STENCIL",
        "COLOR",
      };

      constexpr std::string_view kValidExecutorNames[] = {
        "draw_scene",
        "fullscreen_quad",
        "compute_dispatch",
        "window_sized_compute_dispatch",
        "voxelize",
        "noop",
        "script",
      };

      constexpr std::string_view kValidParamNames[] = {
        "barrier",
        "groups",
        "voxel_dim",
      };

      constexpr std::string_view kValidComputeBarrierValues[] = {
        "shader_image_access",
        "shader_storage",
        "uniform",
        "all",
        "none",
      };

      constexpr std::string_view kValidAccessFlags[] = {
        "read",
        "write",
        "read_write",
      };

      template <size_t N>
      constexpr bool is_valid_enum(const std::string_view value, const std::string_view (&valid)[N]) {
        for (const auto& v : valid) {
          if (value == v) {
            return true;
          }
        }
        return false;
      }

      void validate_resources(validation_result& result, const toml::table& tbl, std::set<std::string>& out_buffer_names,
                              std::set<std::string>& out_texture_names, std::set<std::string>& out_shader_names) {
        const auto buffers = tbl.at_path("resources.buffers");
        const auto textures = tbl.at_path("resources.textures");
        const auto shaders = tbl.at_path("resources.shaders");

        if (!buffers || !buffers.is_array_of_tables()) {
          result.fail("resources.buffers: missing or not an array of tables");
        }
        if (!textures || !textures.is_array_of_tables()) {
          result.fail("resources.textures: missing or not an array of tables");
        }
        if (!shaders || !shaders.is_array_of_tables()) {
          result.fail("resources.shaders: missing or not an array of tables");
        }

        if (buffers && buffers.is_array_of_tables()) {
          for (const auto& buf : *buffers.as_array()) {
            const auto name = buf.at_path("name");
            const auto type = buf.at_path("type");
            const auto usage = buf.at_path("usage");
            const auto tag = buf.at_path("tag");

            if (!name.is_string()) {
              result.fail("resources.buffers[]: 'name' must be a string");
              continue;
            }
            const std::string name_str = name.as_string()->get();
            if (name_str.empty()) {
              result.fail("resources.buffers[]: 'name' must not be empty");
            }

            if (!type.is_string()) {
              result.fail(std::format("resources.buffers['{}']: 'type' must be a string", name_str));
            } else if (!is_valid_enum(type.as_string()->get(), kValidBufferTypes)) {
              result.fail(std::format("resources.buffers['{}']: '{}' is not a valid buffer type", name_str, type.as_string()->get()));
            }

            if (!usage.is_string()) {
              result.fail(std::format("resources.buffers['{}']: 'usage' must be a string", name_str));
            } else if (!is_valid_enum(usage.as_string()->get(), kValidBufferUsages)) {
              result.fail(std::format("resources.buffers['{}']: '{}' is not a valid buffer usage", name_str, usage.as_string()->get()));
            }

            if (!tag.is_string()) {
              result.fail(std::format("resources.buffers['{}']: 'tag' must be a string", name_str));
            }

            out_buffer_names.insert(name_str);
          }
        }

        if (textures && textures.is_array_of_tables()) {
          for (const auto& tex : *textures.as_array()) {
            const auto name = tex.at_path("name");
            const auto use_window_size = tex.at_path("use_window_size");
            const auto type = tex.at_path("type");
            const auto format = tex.at_path("format");
            const auto mip_levels = tex.at_path("mip_levels");
            const auto generate_mips = tex.at_path("generate_mips");
            const auto seed_texture_path = tex.at_path("seed_texture_path");

            if (!name.is_string()) {
              result.fail("resources.textures[]: 'name' must be a string");
              continue;
            }
            const std::string name_str = name.as_string()->get();
            if (name_str.empty()) {
              result.fail("resources.textures[]: 'name' must not be empty");
            }

            if (!use_window_size.is_boolean()) {
              result.fail(std::format("resources.textures['{}']: 'use_window_size' must be a boolean", name_str));
            }

            if (!type.is_string()) {
              result.fail(std::format("resources.textures['{}']: 'type' must be a string", name_str));
            } else if (!is_valid_enum(type.as_string()->get(), kValidTextureTypes)) {
              result.fail(std::format("resources.textures['{}']: '{}' is not a valid texture type", name_str, type.as_string()->get()));
            }

            if (!format.is_string()) {
              result.fail(std::format("resources.textures['{}']: 'format' must be a string", name_str));
            } else if (!is_valid_enum(format.as_string()->get(), kValidTextureFormats)) {
              result.fail(std::format("resources.textures['{}']: '{}' is not a valid texture format", name_str, format.as_string()->get()));
            }

            if (mip_levels) {
              if (!mip_levels.is_integer()) {
                result.fail(std::format("resources.textures['{}']: 'mip_levels' must be an integer", name_str));
              }
            }
            if (generate_mips) {
              if (!generate_mips.is_boolean()) {
                result.fail(std::format("resources.textures['{}']: 'generate_mips' must be a boolean", name_str));
              }
            }
            if (seed_texture_path) {
              if (!seed_texture_path.is_string()) {
                result.fail(std::format("resources.textures['{}']: 'seed_texture_path' must be a string", name_str));
              }
            }

            const auto size = tex.at_path("size");
            if (size) {
              if (!size.is_table()) {
                result.fail(std::format("resources.textures['{}']: 'size' must be a table", name_str));
              } else {
                const auto x = size.at_path("x");
                const auto y = size.at_path("y");
                if (!x || !x.is_integer()) {
                  result.fail(std::format("resources.textures['{}'].size: 'x' must be an integer", name_str));
                }
                if (!y || !y.is_integer()) {
                  result.fail(std::format("resources.textures['{}'].size: 'y' must be an integer", name_str));
                }
              }
            }

            const auto depth = tex.at_path("depth");
            if (depth && !depth.is_integer()) {
              result.fail(std::format("resources.textures['{}']: 'depth' must be an integer", name_str));
            }

            out_texture_names.insert(name_str);
          }
        }

        if (shaders && shaders.is_array_of_tables()) {
          for (const auto& shd : *shaders.as_array()) {
            const auto name = shd.at_path("name");
            const auto compute_path = shd.at_path("compute_path");

            if (!name.is_string()) {
              result.fail("resources.shaders[]: 'name' must be a string");
              continue;
            }
            const std::string name_str = name.as_string()->get();
            if (name_str.empty()) {
              result.fail("resources.shaders[]: 'name' must not be empty");
            }

            if (compute_path) {
              if (!compute_path.is_string()) {
                result.fail(std::format("resources.shaders['{}']: 'compute_path' must be a string", name_str));
              }
            } else {
              const auto vertex_path = shd.at_path("vertex_path");
              const auto fragment_path = shd.at_path("fragment_path");
              if (!vertex_path.is_string()) {
                result.fail(std::format("resources.shaders['{}']: 'vertex_path' must be a string (required for non-compute shaders)", name_str));
              }
              if (!fragment_path.is_string()) {
                result.fail(std::format("resources.shaders['{}']: 'fragment_path' must be a string (required for non-compute shaders)", name_str));
              }
              const auto geometry_path = shd.at_path("geometry_path");
              if (geometry_path && !geometry_path.is_string()) {
                result.fail(std::format("resources.shaders['{}']: 'geometry_path' must be a string", name_str));
              }
            }

            out_shader_names.insert(name_str);
          }
        }
      }

      void validate_frame_bindings(validation_result& result, const toml::table& tbl, std::set<std::string>& out_binding_names) {
        const auto frame_bindings = tbl.at_path("frame.bindings");
        if (!frame_bindings || !frame_bindings.is_array_of_tables()) {
          result.fail("frame.bindings: missing or not an array of tables");
          return;
        }

        for (const auto& b : *frame_bindings.as_array()) {
          const auto name = b.at_path("name");
          const auto tag = b.at_path("tag");
          const auto scope = b.at_path("scope");
          const auto type = b.at_path("type");
          const auto element_size = b.at_path("element_size");

          if (!name.is_string()) {
            result.fail("frame.bindings[]: 'name' must be a string");
            continue;
          }
          const std::string name_str = name.as_string()->get();
          if (name_str.empty()) {
            result.fail("frame.bindings[]: 'name' must not be empty");
          }

          if (!tag.is_string()) {
            result.fail(std::format("frame.bindings['{}']: 'tag' must be a string", name_str));
          }

          if (!scope.is_string()) {
            result.fail(std::format("frame.bindings['{}']: 'scope' must be a string", name_str));
          } else if (!is_valid_enum(scope.as_string()->get(), kValidBindingScopes)) {
            result.fail(std::format("frame.bindings['{}']: '{}' is not a valid binding scope", name_str, scope.as_string()->get()));
          }

          if (!type.is_string()) {
            result.fail(std::format("frame.bindings['{}']: 'type' must be a string", name_str));
          } else if (!is_valid_enum(type.as_string()->get(), kValidBindingTypes)) {
            result.fail(std::format("frame.bindings['{}']: '{}' is not a valid binding type", name_str, type.as_string()->get()));
          }

          if (!element_size || !element_size.is_number()) {
            result.fail(std::format("frame.bindings['{}']: 'element_size' must be an integer", name_str));
          }

          const auto binding_slot = b.at_path("binding");
          if (binding_slot && !binding_slot.is_integer()) {
            result.fail(std::format("frame.bindings['{}']: 'binding' must be an integer if present", name_str));
          }

          out_binding_names.insert(name_str);
        }
      }

      void validate_frame_inputs_outputs(validation_result& result, const toml::table& tbl) {
        const auto inputs = tbl.at_path("frame.inputs");
        const auto outputs = tbl.at_path("frame.outputs");

        if (!inputs || !inputs.is_array_of_tables()) {
          result.fail("frame.inputs: missing or not an array of tables");
        }
        if (!outputs || !outputs.is_array_of_tables()) {
          result.fail("frame.outputs: missing or not an array of tables");
        }

        const auto validate_io_entry = [&](const std::string_view section, const toml::node& entry) {
          const auto pass_name = entry.at_path("pass_name");
          const auto resource_name = entry.at_path("resource_name");

          if (!pass_name.is_string()) {
            result.fail(std::format("{}: each entry must have a string 'pass_name'", section));
            return;
          }
          if (!resource_name.is_string()) {
            result.fail(std::format("{} (pass '{}'): each entry must have a string 'resource_name'",
                                    section, pass_name.as_string()->get()));
            return;
          }

          const std::string pass_str = pass_name.as_string()->get();
          const std::string res_str = resource_name.as_string()->get();

          const auto attachment = entry.at_path("attachment");
          if (attachment) {
            if (!attachment.is_string()) {
              result.fail(std::format("{} (pass '{}', resource '{}'): 'attachment' must be a string", section, pass_str, res_str));
            } else if (!is_valid_enum(attachment.as_string()->get(), kValidAttachmentTypes)) {
              result.fail(std::format("{} (pass '{}', resource '{}'): '{}' is not a valid attachment type",
                                      section, pass_str, res_str, attachment.as_string()->get()));
            }
          }

          const auto binding = entry.at_path("binding");
          if (binding && !binding.is_integer()) {
            result.fail(std::format("{} (pass '{}', resource '{}'): 'binding' must be an integer", section, pass_str, res_str));
          }

          const auto mip_level = entry.at_path("mip_level");
          if (mip_level && !mip_level.is_integer()) {
            result.fail(std::format("{} (pass '{}', resource '{}'): 'mip_level' must be an integer", section, pass_str, res_str));
          }

          const auto access = entry.at_path("access");
          if (access && !access.is_string()) {
            result.fail(std::format("{} (pass '{}', resource '{}'): 'access' must be a string", section, pass_str, res_str));
          } else if (access && !is_valid_enum(access.as_string()->get(), kValidAccessFlags)) {
            result.fail(std::format("{} (pass '{}', resource '{}'): '{}' is not a valid access flag",
                                    section, pass_str, res_str, access.as_string()->get()));
          }
        };

        if (inputs && inputs.is_array_of_tables()) {
          for (const auto& entry : *inputs.as_array()) {
            validate_io_entry("frame.inputs", entry);
          }
        }
        if (outputs && outputs.is_array_of_tables()) {
          for (const auto& entry : *outputs.as_array()) {
            validate_io_entry("frame.outputs", entry);
          }
        }
      }

      void validate_frame_executors(validation_result& result, const toml::table& tbl) {
        const auto executors = tbl.at_path("frame.executors");
        if (!executors || !executors.is_array_of_tables()) {
          result.fail("frame.executors: missing or not an array of tables");
          return;
        }

        for (const auto& exec : *executors.as_array()) {
          const auto pass_name = exec.at_path("pass_name");
          const auto name = exec.at_path("name");

          if (!pass_name.is_string()) {
            result.fail("frame.executors[]: 'pass_name' must be a string");
            continue;
          }
          if (!name.is_string()) {
            result.fail(std::format("frame.executors (pass '{}'): 'name' must be a string", pass_name.as_string()->get()));
            continue;
          }

          const std::string pass_str = pass_name.as_string()->get();
          const std::string name_str = name.as_string()->get();

          if (!is_valid_enum(name_str, kValidExecutorNames)) {
            result.fail(std::format("frame.executors (pass '{}'): '{}' is not a valid executor name", pass_str, name_str));
          }

          const auto uniforms = exec.at_path("uniforms");
          if (uniforms) {
            if (!uniforms.is_array_of_tables()) {
              result.fail(std::format("frame.executors (pass '{}'): 'uniforms' must be an array of tables", pass_str));
            } else {
              for (const auto& u : *uniforms.as_array()) {
                const auto uname = u.at_path("name");
                const auto uvalue = u.at_path("value");
                if (!uname.is_string()) {
                  result.fail(std::format("frame.executors (pass '{}', uniforms): each entry must have a string 'name'", pass_str));
                  continue;
                }
                const std::string uname_str = uname.as_string()->get();
                if (!uvalue || !(uvalue.is_number() || uvalue.is_floating_point() || uvalue.is_boolean())) {
                  result.fail(std::format("frame.executors (pass '{}', uniform '{}'): 'value' must be a number or boolean", pass_str, uname_str));
                }
              }
            }
          }

          const auto params = exec.at_path("params");
          if (params) {
            if (!params.is_array_of_tables()) {
              result.fail(std::format("frame.executors (pass '{}'): 'params' must be an array of tables", pass_str));
            } else {
              for (const auto& p : *params.as_array()) {
                const auto pname = p.at_path("name");
                const auto pvalue = p.at_path("value");
                if (!pname.is_string()) {
                  result.fail(std::format("frame.executors (pass '{}', params): each entry must have a string 'name'", pass_str));
                  continue;
                }
                const std::string pname_str = pname.as_string()->get();
                if (!is_valid_enum(pname_str, kValidParamNames)) {
                  result.fail(std::format("frame.executors (pass '{}', param '{}'): not a valid parameter name", pass_str, pname_str));
                  continue;
                }
                if (!pvalue) {
                  result.fail(std::format("frame.executors (pass '{}', param '{}'): 'value' is required", pass_str, pname_str));
                  continue;
                }
                if (pname_str == "barrier") {
                  if (!pvalue.is_string()) {
                    result.fail(std::format("frame.executors (pass '{}', param 'barrier'): value must be a string", pass_str));
                  } else if (!is_valid_enum(pvalue.as_string()->get(), kValidComputeBarrierValues)) {
                    result.fail(std::format("frame.executors (pass '{}', param 'barrier'): '{}' is not a valid compute barrier value",
                                            pass_str, pvalue.as_string()->get()));
                  }
                } else if (pname_str == "groups") {
                  if (!pvalue.is_array()) {
                    result.fail(std::format("frame.executors (pass '{}', param 'groups'): value must be an array of 3 numbers", pass_str));
                  } else {
                    const auto* arr = pvalue.as_array();
                    if (arr->size() != 3) {
                      result.fail(std::format("frame.executors (pass '{}', param 'groups'): array must have exactly 3 elements", pass_str));
                    } else {
                      for (size_t i = 0; i < 3; ++i) {
                        if (!arr->at(i).is_number()) {
                          result.fail(std::format("frame.executors (pass '{}', param 'groups'): element [{}] must be a number", pass_str, i));
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }

      void validate_frame_resource_tags(validation_result& result, const toml::table& tbl) {
        const auto resource_tags = tbl.at_path("frame.resource_tags");
        if (!resource_tags || !resource_tags.is_array_of_tables()) {
          result.fail("frame.resource_tags: missing or not an array of tables");
          return;
        }

        for (const auto& rt : *resource_tags.as_array()) {
          const auto name = rt.at_path("name");
          const auto type = rt.at_path("type");
          if (!name.is_string()) {
            result.fail("frame.resource_tags[]: 'name' must be a string");
            continue;
          }
          if (!type.is_string()) {
            result.fail(std::format("frame.resource_tags['{}']: 'type' must be a string", name.as_string()->get()));
          }
        }
      }

      void validate_frame_passes(validation_result& result, const toml::table& tbl, std::set<std::string>& out_pass_names) {
        const auto passes = tbl.at_path("frame.passes");
        if (!passes || !passes.is_array_of_tables()) {
          result.fail("frame.passes: missing or not an array of tables");
          return;
        }

        for (const auto& pass : *passes.as_array()) {
          const auto name = pass.at_path("name");
          const auto pass_type = pass.at_path("pass_type");
          const auto shader_name = pass.at_path("shader_name");

          if (!name.is_string()) {
            result.fail("frame.passes[]: 'name' must be a string");
            continue;
          }
          const std::string name_str = name.as_string()->get();
          if (name_str.empty()) {
            result.fail("frame.passes[]: 'name' must not be empty");
          }

          if (!pass_type.is_string()) {
            result.fail(std::format("frame.passes['{}']: 'pass_type' must be a string", name_str));
          } else if (!is_valid_enum(pass_type.as_string()->get(), kValidPassTypes)) {
            result.fail(std::format("frame.passes['{}']: '{}' is not a valid pass type", name_str, pass_type.as_string()->get()));
          }

          if (!shader_name.is_string()) {
            result.fail(std::format("frame.passes['{}']: 'shader_name' must be a string", name_str));
          }

          const auto samples = pass.at_path("samples");
          const auto multisample = pass.at_path("multisample");
          if (samples && !samples.is_integer()) {
            result.fail(std::format("frame.passes['{}']: 'samples' must be an integer", name_str));
          }
          if (samples && multisample && (!multisample.is_boolean() || !samples.is_integer())) {
            result.fail(std::format("frame.passes['{}']: 'multisample' and 'samples' must be a boolean and an integer respectively", name_str));
          }
          if (!samples && multisample && !multisample.is_integer()) {
            result.fail(std::format("frame.passes['{}']: 'multisample' does not specify number of samples", name_str));
          }

          const auto clear_color = pass.at_path("clear_color");
          if (clear_color) {
            if (!clear_color.is_array()) {
              result.fail(std::format("frame.passes['{}']: 'clear_color' must be an array", name_str));
            } else {
              const auto* cc = clear_color.as_array();
              if (cc->size() < 3 || cc->size() > 4) {
                result.fail(std::format("frame.passes['{}']: 'clear_color' must have 3 or 4 elements, found {}", name_str, cc->size()));
              } else {
                for (size_t i = 0; i < cc->size(); ++i) {
                  if (!cc->at(i).is_number() && !cc->at(i).is_floating_point()) {
                    result.fail(std::format("frame.passes['{}'].clear_color[{}]: must be a number", name_str, i));
                  }
                }
              }
            }
          }

          const auto create_framebuffer = pass.at_path("create_framebuffer");
          if (create_framebuffer && !create_framebuffer.is_boolean()) {
            result.fail(std::format("frame.passes['{}']: 'create_framebuffer' must be a boolean", name_str));
          }

          const auto depends_on = pass.at_path("depends_on");
          if (depends_on && !depends_on.is_array()) {
            result.fail(std::format("frame.passes['{}']: 'depends_on' must be a string", name_str));
          } else if (depends_on) {
            for (size_t i = 0; i < depends_on.as_array()->size(); ++i) {
              if (!depends_on.as_array()->at(i).is_string()) {
                result.fail(std::format("frame.passes['{}']: 'depends_on[{}]' must be a string", name_str, i));
              }
            }
          }

          out_pass_names.insert(name_str);
        }

        if (result.valid) {
          for (const auto& pas : *passes.as_array()) {
            const auto depends_on = pas.at_path("depends_on");
            if (depends_on) {
              std::string name = pas.at_path("name").as_string()->get();
              // we know is array because result.valid is still true
              for (size_t i = 0; i < depends_on.as_array()->size(); ++i) {
                const auto depends_on_str = depends_on.as_array()->at(i).as_string()->get();
                if (out_pass_names.find(depends_on_str) == out_pass_names.end()) {
                  result.fail(std::format("frame.passes['{}']: 'depends_on' references unknown pass '{}'", name, depends_on_str));
                }
              }
            }
          }
        }
      }

      void validate_frame_pass_bindings(validation_result& result, const toml::table& tbl) {
        const auto pass_bindings = tbl.at_path("frame.pass-bindings");
        if (!pass_bindings) {
          return;
        }
        if (!pass_bindings.is_array_of_tables()) {
          result.fail("frame.pass-bindings: must be an array of tables if present");
          return;
        }

        for (const auto& pb : *pass_bindings.as_array()) {
          const auto pass_name = pb.at_path("pass_name");
          const auto name = pb.at_path("name");
          const auto binding = pb.at_path("binding");

          if (!pass_name.is_string()) {
            result.fail("frame.pass-bindings[]: 'pass_name' must be a string");
            continue;
          }
          if (!name.is_string()) {
            result.fail(std::format("frame.pass-bindings (pass '{}'): 'name' must be a string", pass_name.as_string()->get()));
            continue;
          }
          if (!binding.is_integer()) {
            result.fail(std::format("frame.pass-bindings (pass '{}', name '{}'): 'binding' must be an integer",
                                    pass_name.as_string()->get(), name.as_string()->get()));
          }
        }
      }

      void validate_cross_references(validation_result& result, const toml::table& tbl,
                                     const std::set<std::string>& buffer_names, const std::set<std::string>& texture_names, const std::set<std::string>& shader_names, const std::set<std::string>& binding_names, const std::set<std::string>& pass_names) {
        const auto resource_exists = [&](const std::string& n) {
          return buffer_names.contains(n) || texture_names.contains(n);
        };

        // passes -> shaders
        const auto passes = tbl.at_path("frame.passes");
        if (passes && passes.is_array_of_tables()) {
          for (const auto& pass : *passes.as_array()) {
            const auto name = pass.at_path("name");
            const auto shader_name = pass.at_path("shader_name");
            if (name.is_string() && shader_name.is_string()) {
              const std::string shader_str = shader_name.as_string()->get();
              if (!shader_names.contains(shader_str)) {
                result.fail(std::format("frame.passes['{}']: shader '{}' is not declared in resources.shaders",
                                        name.as_string()->get(), shader_str));
              }
            }
          }
        }

        // inputs/outputs -> passes and resources
        const auto validate_io_refs = [&](const std::string_view section, const toml::array& arr) {
          for (const auto& entry : arr) {
            const auto pass_name = entry.at_path("pass_name");
            const auto resource_name = entry.at_path("resource_name");
            if (!pass_name.is_string() || !resource_name.is_string()) {
              continue;
            }
            const std::string pass_str = pass_name.as_string()->get();
            const std::string res_str = resource_name.as_string()->get();
            if (!pass_names.contains(pass_str)) {
              result.fail(std::format("{} (pass '{}', resource '{}'): pass is not declared in frame.passes", section, pass_str, res_str));
            }
            if (!resource_exists(res_str)) {
              result.fail(std::format("{} (pass '{}', resource '{}'): resource is not declared in resources.buffers or resources.textures",
                                      section, pass_str, res_str));
            }
          }
        };

        const auto inputs = tbl.at_path("frame.inputs");
        if (inputs && inputs.is_array_of_tables()) {
          validate_io_refs("frame.inputs", *inputs.as_array());
        }
        const auto outputs = tbl.at_path("frame.outputs");
        if (outputs && outputs.is_array_of_tables()) {
          validate_io_refs("frame.outputs", *outputs.as_array());
        }

        // executors -> passes
        const auto executors = tbl.at_path("frame.executors");
        if (executors && executors.is_array_of_tables()) {
          for (const auto& exec : *executors.as_array()) {
            const auto pass_name = exec.at_path("pass_name");
            if (!pass_name.is_string()) {
              continue;
            }
            const std::string pass_str = pass_name.as_string()->get();
            if (!pass_names.contains(pass_str)) {
              result.fail(std::format("frame.executors (pass '{}'): pass is not declared in frame.passes", pass_str));
            }
          }
        }

        // pass-bindings -> passes and frame.bindings
        const auto pass_bindings = tbl.at_path("frame.pass-bindings");
        if (pass_bindings && pass_bindings.is_array_of_tables()) {
          for (const auto& pb : *pass_bindings.as_array()) {
            const auto pass_name = pb.at_path("pass_name");
            const auto name = pb.at_path("name");
            if (!pass_name.is_string() || !name.is_string()) {
              continue;
            }
            const std::string pass_str = pass_name.as_string()->get();
            const std::string binding_str = name.as_string()->get();
            if (!pass_names.contains(pass_str)) {
              result.fail(std::format("frame.pass-bindings (pass '{}', binding '{}'): pass is not declared in frame.passes",
                                      pass_str, binding_str));
            }
            if (!binding_names.contains(binding_str)) {
              result.fail(std::format("frame.pass-bindings (pass '{}', binding '{}'): binding is not declared in frame.bindings",
                                      pass_str, binding_str));
            }
          }
        }
      }

    }  // namespace
  }  // namespace detail
}  // namespace other