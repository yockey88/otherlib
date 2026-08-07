/**
 * \file tools/model_cli_tool.cpp
 **/
#include "tools/model_cli_tool.hpp"

#include <filesystem>
#include <fstream>

#include "core/profiler.hpp"

#include "model/model_importer.hpp"

#include "serialization/animation_serializer.hpp"

#include "tools/scene_cli_tool.hpp"

namespace other {
  namespace cli {
    namespace {

      constexpr std::string_view kUsage =
        "usage: oecli model <command> <input>\n"
        "\n"
        "commands:\n"
        "  info <input.fbx|.obj|.gltf|.glb|.dae|.3ds>            import a model headless and print its summary\n"
        "  extract-clips <input> [-o|--output <dir>]             write each embedded animation clip to <stem>@<clip>.oanim\n"
        "                                                        (default output directory: the model file's directory)";

      constexpr std::string_view kAnimUsage =
        "usage: oecli anim <command> <input>\n"
        "\n"
        "commands:\n"
        "  info <input.oanim>   parse a standalone animation clip and print its track summary";

      /// clip names come from dcc tools and routinely carry characters paths cannot ('Armature|Dance');
      ///  utf-8 multibyte sequences pass through untouched
      std::string filesystem_safe(const std::string_view name) {
        std::string safe{ name };
        for (char& c : safe) {
          if (static_cast<unsigned char>(c) < 0x20 || std::string_view{ "\\/:*?\"<>|" }.find(c) != std::string_view::npos) {
            c = '_';
          }
        }
        return safe;
      }

      tool_result info(tool_context& ctx, const filepath& input) {
        PROFILE_SECTION("model_tool::info");
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

        ctx.print("  skeleton: {} joints", data.skel.joints.size());
        if (!data.skel.empty() && data.skel.root_transform != glm::mat4(1.f)) {
          const glm::mat4& rt = data.skel.root_transform;
          for (int r = 0; r < 4; ++r) {
            ctx.print("    root_transform[{}] = [{:.4f}, {:.4f}, {:.4f}, {:.4f}]", r, rt[0][r], rt[1][r], rt[2][r], rt[3][r]);
          }
        }
        for (size_t i = 0; i < data.skel.joints.size(); ++i) {
          const joint& j = data.skel.joints[i];
          if (j.parent == -1) {
            /// full bind TRS for roots — the fastest place to spot convention problems
            ctx.print("    [{}] '{}' (parent {}) | bind pos ({:.3f}, {:.3f}, {:.3f}) rot wxyz ({:.4f}, {:.4f}, {:.4f}, {:.4f}) scale ({:.3f}, {:.3f}, {:.3f})",
                      i, j.name, j.parent,
                      j.bind_position.x, j.bind_position.y, j.bind_position.z,
                      j.bind_rotation.w, j.bind_rotation.x, j.bind_rotation.y, j.bind_rotation.z,
                      j.bind_scale.x, j.bind_scale.y, j.bind_scale.z);
          } else {
            ctx.print("    [{}] '{}' (parent {})", i, j.name, j.parent);
          }
        }

        ctx.print("  clips: {}", data.clips.size());
        for (size_t i = 0; i < data.clips.size(); ++i) {
          const animation_clip& clip = data.clips[i];
          ctx.print("    [{}] '{}': {:.3f}s, {} tracks", i, clip.name, clip.duration, clip.joint_tracks.size());
          /// root-track first keys compare directly against the root bind TRS above
          for (const joint_track& track : clip.joint_tracks) {
            const int16_t joint_idx = data.skel.find_joint(track.joint_name_hash);
            if (joint_idx < 0 || data.skel.joints[joint_idx].parent != -1) {
              continue;
            }
            if (!track.rotation_keyframes.empty()) {
              const glm::quat& q = track.rotation_keyframes.front().value;
              ctx.print("      root track '{}': rot[0] wxyz ({:.4f}, {:.4f}, {:.4f}, {:.4f})", track.joint_name, q.w, q.x, q.y, q.z);
            }
            if (!track.position_keyframes.empty()) {
              const glm::vec3& p = track.position_keyframes.front().value;
              ctx.print("      root track '{}': pos[0] ({:.3f}, {:.3f}, {:.3f})", track.joint_name, p.x, p.y, p.z);
            }
          }
        }
        return tool_result::ok();
      }

      tool_result extract_clips(tool_context& ctx, const filepath& input, const filepath& output_dir) {
        PROFILE_SECTION("model_tool::extract_clips");
        model_import_result result = import(input);
        for (const std::string& warning : result.warnings) {
          ctx.print("warning: {}", warning);
        }
        if (!result.data.has_value()) {
          return tool_result::error(result.error);
        }

        const model_data& data = *result.data;
        if (data.clips.empty()) {
          ctx.print("'{}' contains no animation clips", input.string());
          return tool_result::ok();
        }

        std::error_code ec;
        std::filesystem::create_directories(output_dir, ec);
        if (ec) {
          return tool_result::error(std::format("could not create output directory '{}': {}", output_dir.string(), ec.message()));
        }

        const std::string stem = input.filename().stem().string();
        for (const animation_clip& clip : data.clips) {
          PROFILE_SECTION("model_tool::extract_clips--write-clip");
          const filepath out_path = output_dir / std::format("{}@{}{}", stem, filesystem_safe(clip.name), serialization::kAnimationClipExtension);
          const ostd::vector<uint8_t> bytes = serialization::serialize_animation_clip(clip);

          std::ofstream out(out_path, std::ios::binary | std::ios::trunc);
          if (!out || !out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()))) {
            return tool_result::error(std::format("failed to write '{}'", out_path.string()));
          }
          ctx.print("{} ({} tracks, {:.3f}s)", out_path.string(), clip.joint_tracks.size(), clip.duration);
        }
        return tool_result::ok();
      }

      tool_result anim_info(tool_context& ctx, const filepath& input) {
        PROFILE_SECTION("anim_tool::anim_info");
        serialization::clip_parse_result parsed = serialization::load_animation_clip(input);
        if (!parsed.success()) {
          return tool_result::error(parsed.error);
        }

        const animation_clip& clip = *parsed.clip;
        ctx.print("clip '{}'", clip.name);
        ctx.print("  duration: {:.3f}s", clip.duration);
        ctx.print("  tracks: {}", clip.joint_tracks.size());
        for (size_t i = 0; i < clip.joint_tracks.size(); ++i) {
          const joint_track& track = clip.joint_tracks[i];
          ctx.print("    [{}] '{}': {} position, {} rotation, {} scale keys", i, track.joint_name,
                    track.position_keyframes.size(), track.rotation_keyframes.size(), track.scale_keyframes.size());
        }
        return tool_result::ok();
      }

    }  // namespace

    std::string_view model_tool::usage() const {
      return kUsage;
    }

    tool_result model_tool::execute(tool_context& ctx, std::span<const std::string> args) {
      PROFILE_SECTION("model_tool::execute");
      ensure_cli_runtime();

      if (args.empty()) {
        return tool_result::error(std::format("missing command\n{}", kUsage));
      }

      const std::string& command = args[0];
      if (command == "info") {
        if (args.size() != 2) {
          return tool_result::error(std::format("'info' expects exactly one input file\n{}", kUsage));
        }

        filepath input = filepath(args[1]);
        if (!input.is_absolute()) {
          input = ctx.working_directory / input;
        }
        return info(ctx, input);
      }

      if (command == "extract-clips") {
        filepath input;
        filepath output_dir;
        for (size_t i = 1; i < args.size(); ++i) {
          if (args[i] == "-o" || args[i] == "--output") {
            if (i + 1 >= args.size()) {
              return tool_result::error(std::format("'{}' expects a directory\n{}", args[i], kUsage));
            }
            output_dir = filepath(args[++i]);
          } else if (input.empty()) {
            input = filepath(args[i]);
          } else {
            return tool_result::error(std::format("unexpected argument '{}'\n{}", args[i], kUsage));
          }
        }
        if (input.empty()) {
          return tool_result::error(std::format("'extract-clips' expects an input file\n{}", kUsage));
        }

        if (!input.is_absolute()) {
          input = ctx.working_directory / input;
        }
        if (output_dir.empty()) {
          output_dir = input.parent_path();
        } else if (!output_dir.is_absolute()) {
          output_dir = ctx.working_directory / output_dir;
        }
        return extract_clips(ctx, input, output_dir);
      }

      return tool_result::error(std::format("unknown command '{}'\n{}", command, kUsage));
    }

    std::string_view anim_tool::usage() const {
      return kAnimUsage;
    }

    tool_result anim_tool::execute(tool_context& ctx, std::span<const std::string> args) {
      PROFILE_SECTION("anim_tool::execute");
      ensure_cli_runtime();

      if (args.empty()) {
        return tool_result::error(std::format("missing command\n{}", kAnimUsage));
      }
      if (args[0] != "info") {
        return tool_result::error(std::format("unknown command '{}'\n{}", args[0], kAnimUsage));
      }
      if (args.size() != 2) {
        return tool_result::error(std::format("'info' expects exactly one input file\n{}", kAnimUsage));
      }

      filepath input = filepath(args[1]);
      if (!input.is_absolute()) {
        input = ctx.working_directory / input;
      }
      return anim_info(ctx, input);
    }

  }  // namespace cli
}  // namespace other
