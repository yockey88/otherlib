/**
 * \file tools/model_cli_tool.cpp
 **/
#include "tools/model_cli_tool.hpp"

#include <filesystem>

#include "model/model_importer.hpp"

#include "tools/scene_cli_tool.hpp"

namespace other {
  namespace cli {
    namespace {

      constexpr std::string_view kUsage =
        "usage: oecli model <command> <input>\n"
        "\n"
        "commands:\n"
        "  info <input.fbx|.obj|.gltf|.glb|.dae|.3ds>   import a model headless and print its summary";

      tool_result info(tool_context& ctx, const filepath& input) {
        model_import_result result = import(input);
        for (const std::string& warning : result.warnings) {
          ctx.print("warning: {}", warning);
        }
        if (!result.data.has_value()) {
          return tool_result::error(result.error);
        }

        const model_data& data = *result.data;
        ctx.print("model '{}'", data.name);
        ctx.print("  vertices: {}, triangles: {}", data.vertices.size(), data.indices.size());
        ctx.print("  bounds: [{}, {}, {}] - [{}, {}, {}]", data.bounds.min.x, data.bounds.min.y, data.bounds.min.z, data.bounds.max.x, data.bounds.max.y, data.bounds.max.z);

        ctx.print("  submeshes: {}", data.submeshes.size());
        for (size_t i = 0; i < data.submeshes.size(); ++i) {
          const submesh& sm = data.submeshes[i];
          ctx.print("    [{}] '{}': {} vertices, {} indices, material {}{}", i, sm.name, sm.vert_cnt, sm.idx_cnt, sm.material_index, sm.rigged ? ", rigged" : "");
        }

        ctx.print("  nodes: {}", data.nodes.size());

        ctx.print("  materials: {}", data.materials.size());
        for (size_t i = 0; i < data.materials.size(); ++i) {
          const imported_material& mat = data.materials[i];
          std::string textures = "";
          const auto append_texture = [&textures](const std::string_view slot, const std::string& path) {
            if (!path.empty()) {
              textures += std::format("{}{}: '{}'", textures.empty() ? "" : ", ", slot, path);
            }
          };
          append_texture("base", mat.base_color_texture);
          append_texture("normal", mat.normal_texture);
          append_texture("metallic-roughness", mat.metallic_roughness_texture);
          append_texture("emissive", mat.emissive_texture);
          ctx.print("    [{}] '{}': roughness {}, metalness {}{}", i, mat.name, mat.roughness, mat.metalness,
                    textures.empty() ? "" : std::format(" | {}", textures));
        }

        ctx.print("  skeleton: {} bones", data.skel.bones.size());
        return tool_result::ok();
      }

    }  // namespace

    std::string_view model_tool::usage() const {
      return kUsage;
    }

    tool_result model_tool::execute(tool_context& ctx, std::span<const std::string> args) {
      ensure_cli_runtime();

      if (args.empty()) {
        return tool_result::error(std::format("missing command\n{}", kUsage));
      }
      if (args[0] != "info") {
        return tool_result::error(std::format("unknown command '{}'\n{}", args[0], kUsage));
      }
      if (args.size() != 2) {
        return tool_result::error(std::format("'info' expects exactly one input file\n{}", kUsage));
      }

      filepath input = filepath(args[1]);
      if (!input.is_absolute()) {
        input = ctx.working_directory / input;
      }
      return info(ctx, input);
    }

  }  // namespace cli
}  // namespace other
