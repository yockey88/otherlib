/**
 * \file tools/material_cli_tool.cpp
 */
#include "tools/material_cli_tool.hpp"

#include <toml++/toml.h>

#include "core/fnv.hpp"
#include "core/profiler.hpp"
#include "file/path_helpers.hpp"

#include "gpu_resource/material.hpp"
#include "renderer/material_layout.hpp"

#include "tools/scene_cli_tool.hpp"

namespace other {
  namespace cli {
    namespace {

      constexpr std::string_view kUsage =
        "usage: oecli material <command> <input> [options]\n"
        "\n"
        "commands:\n"
        "  info <input.omat>                        parse a material and print its params/textures\n"
        "  check <input.omat> -p <pipeline toml>    dry-run the material against a pipeline's [materials.layout]\n"
        "\n"
        "options:\n"
        "  -p, --pipeline <path>   render pipeline TOML whose [materials.layout] to pack against";

      struct parsed_arguments {
        std::string command = "";
        filepath input = "";
        opt<filepath> pipeline = std::nullopt;
        opt<std::string> error = std::nullopt;
      };

      /// same loop as the scene tool's, with -p/--pipeline in place of -o/--out
      parsed_arguments parse_arguments(std::span<const std::string> args) {
        parsed_arguments parsed = {};
        if (args.empty()) {
          parsed.error = "missing command";
          return parsed;
        }
        parsed.command = args[0];

        for (size_t i = 1; i < args.size(); ++i) {
          const std::string& arg = args[i];
          if (arg == "-p" || arg == "--pipeline") {
            if (i + 1 >= args.size()) {
              parsed.error = std::format("'{}' expects a path argument", arg);
              return parsed;
            }
            parsed.pipeline = filepath(args[++i]);
          } else if (!arg.empty() && arg.front() == '-') {
            parsed.error = std::format("unknown option '{}'", arg);
            return parsed;
          } else if (parsed.input.empty()) {
            parsed.input = filepath(arg);
          } else {
            parsed.error = std::format("unexpected argument '{}'", arg);
            return parsed;
          }
        }

        if (parsed.input.empty()) {
          parsed.error = "missing input file";
        }
        return parsed;
      }

      std::string_view kind_name(material_value::kind kind) {
        switch (kind) {
          case material_value::kind::F32: return "float";
          case material_value::kind::VEC2: return "vec2";
          case material_value::kind::VEC3: return "vec3";
          case material_value::kind::VEC4: return "vec4";
          case material_value::kind::I32: return "int";
          case material_value::kind::B32: return "bool";
          default: return "?";
        }
      }

      std::string format_value(const material_value& v) {
        switch (v.value_kind) {
          case material_value::kind::F32: return std::format("{}", v.data.x);
          case material_value::kind::VEC2: return std::format("[{}, {}]", v.data.x, v.data.y);
          case material_value::kind::VEC3: return std::format("[{}, {}, {}]", v.data.x, v.data.y, v.data.z);
          case material_value::kind::VEC4: return std::format("[{}, {}, {}, {}]", v.data.x, v.data.y, v.data.z, v.data.w);
          case material_value::kind::I32:
          case material_value::kind::B32: {
            int32_t i = 0;  /// ints/bools are bit-cast into data.x (material.hpp)
            std::memcpy(&i, &v.data.x, sizeof(i));
            return v.value_kind == material_value::kind::I32 ? std::format("{}", i) : (i != 0 ? "true" : "false");
          }
          default: return "?";
        }
      }

      ostd::vector<std::pair<std::string, material_value>> sorted_params(const material& mat) {
        ostd::vector<std::pair<std::string, material_value>> out;
        for (const auto& [hash, value] : mat.params) {
          const auto name_it = mat.param_names.find(hash);
          out.emplace_back(name_it != mat.param_names.end() ? name_it->second : std::format("{:#018x}", hash), value);
        }
        std::ranges::sort(out, {}, &std::pair<std::string, material_value>::first);
        return out;
      }

      ostd::vector<std::pair<std::string, std::string>> sorted_textures(const material& mat) {
        ostd::vector<std::pair<std::string, std::string>> out;
        for (const auto& [hash, path] : mat.texture_paths) {
          const auto name_it = mat.texture_hashes.find(hash);
          out.emplace_back(name_it != mat.texture_hashes.end() ? std::format("{:#018x}", name_it->second) : std::format("{:#018x}", hash), path);
        }
        std::ranges::sort(out, {}, &std::pair<std::string, std::string>::first);
        return out;
      }

      tool_result info(tool_context& ctx, const filepath& input) {
        PROFILE_SECTION("material_tool::info");
        material_parse_result parsed = parse_material_toml(input);
        for (const std::string& warning : parsed.warnings) {
          ctx.print("warning: {}", warning);
        }
        if (!parsed.success()) {
          return tool_result::error(parsed.error);
        }

        const material& mat = *parsed.mat;
        ctx.print("material '{}'", mat.name);
        ctx.print("  params: {}", mat.params.size());
        for (const auto& [name, value] : sorted_params(mat)) {
          ctx.print("    {}: {} {}", name, kind_name(value.value_kind), format_value(value));
        }
        ctx.print("  textures: {}", mat.texture_paths.size());
        for (const auto& [name, path] : sorted_textures(mat)) {
          ctx.print("    {}: {}", name, path.empty() ? "(none)" : std::format("'{}'", path));
        }
        return tool_result::ok();
      }

      struct layout_read_result {
        opt<material_layout> layout;
        std::string error;
        bool declared = false;  //< false = the pipeline has no [materials.layout] at all
      };

      opt<material_value::kind> kind_from_string(std::string_view str) {
        switch (FNV(str)) {
          case FNV("float"): return material_value::kind::F32;
          case FNV("vec2"): return material_value::kind::VEC2;
          case FNV("vec3"): return material_value::kind::VEC3;
          case FNV("vec4"): return material_value::kind::VEC4;
          case FNV("int"): return material_value::kind::I32;
          case FNV("bool"): return material_value::kind::B32;
          default: return std::nullopt;
        }
      }

      opt<material_value> default_from_node(toml::node_view<const toml::node> node, material_value::kind kind) {
        if (!node) {
          material_value zero{};
          zero.value_kind = kind;
          return zero;
        }
        switch (kind) {
          case material_value::kind::F32:
            if (!node.is_number()) return std::nullopt;
            return material_value::from(static_cast<float>(node.is_floating_point() ? node.as_floating_point()->get() : static_cast<double>(node.as_integer()->get())));
          case material_value::kind::I32:
            if (!node.is_integer()) return std::nullopt;
            return material_value::from(static_cast<int32_t>(node.as_integer()->get()));
          case material_value::kind::B32:
            if (!node.is_boolean()) return std::nullopt;
            return material_value::from(node.as_boolean()->get());
          default: {
            const size_t expected = kind == material_value::kind::VEC2 ? 2 : kind == material_value::kind::VEC3 ? 3 :
                                                                                                                  4;
            const toml::array* arr = node.as_array();
            if (arr == nullptr || arr->size() != expected) return std::nullopt;
            glm::vec4 v(0.f);
            for (size_t i = 0; i < expected; ++i) {
              const toml::node& e = (*arr)[i];
              if (!e.is_number()) return std::nullopt;
              v[static_cast<glm::length_t>(i)] = e.is_floating_point() ? static_cast<float>(e.as_floating_point()->get()) : static_cast<float>(e.as_integer()->get());
            }
            material_value value{};
            value.value_kind = kind;
            value.data = v;
            return value;
          }
        }
      }

      layout_read_result read_layout_declaration(const filepath& pipeline_path) {
        PROFILE_SECTION("material_tool::read_layout_declaration");
        layout_read_result result;

        toml::table table;
        try {
          table = toml::parse_file(pipeline_path.string());
        } catch (const std::exception& e) {
          result.error = std::format("failed to parse pipeline '{}': {}", pipeline_path.string(), e.what());
          return result;
        } catch (...) {
          result.error = std::format("failed to parse pipeline '{}': unknown error", pipeline_path.string());
          return result;
        }

        auto params = table.at_path("materials.layout.params");
        if (!params) {
          return result;  /// declared stays false — the pipeline draws without materials
        }
        result.declared = true;
        if (!params.is_array_of_tables()) {
          result.error = "materials.layout.params must be an array of tables";
          return result;
        }

        material_layout layout;
        for (const auto& p : *params.as_array()) {
          auto p_name = p.at_path("name");
          auto p_type = p.at_path("type");
          if (!p_name || !p_name.is_string() || !p_type || !p_type.is_string()) {
            result.error = "material layout params need string 'name' and 'type' fields";
            return result;
          }

          material_layout::param& param = layout.params.emplace_back();
          param.name = p_name.as_string()->get();
          if (param.name.empty()) {
            result.error = "material layout param with empty name";
            return result;
          }

          const opt<material_value::kind> kind = kind_from_string(p_type.as_string()->get());
          if (!kind.has_value()) {
            result.error = std::format("material layout param '{}': unsupported type '{}' (float, vec2, vec3, vec4, int, bool)",
                                       param.name, p_type.as_string()->get());
            return result;
          }
          param.kind = *kind;

          const opt<material_value> def = default_from_node(p.at_path("default"), param.kind);
          if (!def.has_value()) {
            result.error = std::format("material layout param '{}': default does not match declared type '{}'",
                                       param.name, kind_name(param.kind));
            return result;
          }
          param.default_value = *def;
        }
        for (size_t i = 0; i < layout.params.size(); ++i) {
          for (size_t j = i + 1; j < layout.params.size(); ++j) {
            if (FNV(layout.params[i].name) == FNV(layout.params[j].name)) {
              result.error = std::format("material layout declares param '{}' twice", layout.params[i].name);
              return result;
            }
          }
        }

        if (auto slots = table.at_path("materials.layout.texture-slots"); slots) {
          if (!slots.is_array_of_tables()) {
            result.error = "materials.layout.texture-slots must be an array of tables";
            return result;
          }
          for (const auto& s : *slots.as_array()) {
            auto s_name = s.at_path("name");
            auto s_uniform = s.at_path("uniform");
            auto s_unit = s.at_path("unit");
            if (!s_name || !s_name.is_string() || !s_uniform || !s_uniform.is_string() || !s_unit || !s_unit.is_integer()) {
              result.error = "material layout texture slots need string 'name'/'uniform' and integer 'unit' fields";
              return result;
            }
            material_layout::texture_slot& slot = layout.texture_slots.emplace_back();
            slot.name = s_name.as_string()->get();
            slot.uniform = s_uniform.as_string()->get();
            slot.unit = static_cast<uint32_t>(s_unit.as_integer()->get());
            if (slot.name.empty() || slot.uniform.empty()) {
              result.error = "material layout texture slot needs both a name and a uniform";
              return result;
            }
          }
          for (size_t i = 0; i < layout.texture_slots.size(); ++i) {
            for (size_t j = i + 1; j < layout.texture_slots.size(); ++j) {
              if (FNV(layout.texture_slots[i].name) == FNV(layout.texture_slots[j].name)) {
                result.error = std::format("material layout declares texture slot '{}' twice", layout.texture_slots[i].name);
                return result;
              }
              if (layout.texture_slots[i].unit == layout.texture_slots[j].unit) {
                result.error = std::format("material layout texture slots '{}' and '{}' share unit {}",
                                           layout.texture_slots[i].name, layout.texture_slots[j].name, layout.texture_slots[i].unit);
                return result;
              }
            }
          }
        }

        if (auto capacity = table.at_path("materials.layout.instance-capacity"); capacity) {
          if (!capacity.is_integer() || capacity.as_integer()->get() <= 0) {
            result.error = "materials.layout.instance-capacity must be a positive integer";
            return result;
          }
          layout.instance_capacity = static_cast<uint32_t>(capacity.as_integer()->get());
        }

        /// every finalize() precondition is now guaranteed, so this runs the engine's own
        ///  std430 offset/element_size/base_color math — not a reimplementation
        layout.finalize();
        result.layout = std::move(layout);
        return result;
      }

      tool_result check(tool_context& ctx, const filepath& input, const filepath& pipeline_path) {
        PROFILE_SECTION("material_tool::check");
        material_parse_result parsed = parse_material_toml(input);
        for (const std::string& warning : parsed.warnings) {
          ctx.print("warning: {}", warning);
        }
        if (!parsed.success()) {
          return tool_result::error(parsed.error);
        }
        material& mat = *parsed.mat;
        size_t warning_count = parsed.warnings.size();

        layout_read_result layout_read = read_layout_declaration(pipeline_path);
        if (layout_read.declared && !layout_read.layout.has_value()) {
          return tool_result::error(layout_read.error);
        }
        if (!layout_read.declared) {
          return tool_result::ok(std::format("'{}' declares no [materials.layout] — it draws without materials, so '{}' would be ignored",
                                             pipeline_path.filename().string(), mat.name));
        }
        const material_layout& layout = *layout_read.layout;

        ctx.print("material '{}' vs pipeline '{}'", mat.name, pipeline_path.filename().string());
        ctx.print("  layout: {} params, {} texture slots, element size {} bytes, capacity {}",
                  layout.params.size(), layout.texture_slots.size(), layout.element_size, layout.instance_capacity);

        /// the exact pack the renderer runs per draw (render_pipeline::fill_material_slice),
        ///  with the warning callback collecting instead of logging
        ostd::vector<uint8_t> blob;
        blob.resize(layout.element_size);
        ostd::vector<std::pair<std::string, std::string>> pack_warnings;
        layout.pack(&mat, std::span(blob), [&pack_warnings](std::string_view param, std::string_view reason) {
          pack_warnings.emplace_back(std::string{ param }, std::string{ reason });
        });

        ctx.print("  packed values:");
        for (const material_layout::param& p : layout.params) {
          const auto it = mat.params.find(p.name_hash);
          const bool from_material = it != mat.params.end() && it->second.value_kind == p.kind;
          const material_value& effective = from_material ? it->second : p.default_value;
          ctx.print("    +{:<3} {} {} = {}  ({})", p.offset, kind_name(p.kind), p.name, format_value(effective),
                    from_material ? "material" : "layout default");
        }
        for (const auto& [param, reason] : pack_warnings) {
          ctx.print("  warning: param '{}': {}", param, reason);
        }
        warning_count += pack_warnings.size();

        ctx.print("  texture slots:");
        for (const material_layout::texture_slot& slot : layout.texture_slots) {
          const auto it = mat.texture_paths.find(slot.name_hash);
          if (it == mat.texture_paths.end() || it->second.empty()) {
            /// unset slots bind 1x1 stand-ins (render_pipeline.cpp): flat normal for the
            ///  slot literally named "normal", white for everything else
            ctx.print("    {} (unit {}, {}): unset -> {} fallback", slot.name, slot.unit, slot.uniform,
                      slot.name == "normal" ? "flat-normal" : "white");
            continue;
          }
          /// same resolution the asset resolver uses for .omat texture edges
          const filepath abs = resolve_relative(std::filesystem::absolute(input), it->second);
          const bool exists = std::filesystem::exists(abs);
          ctx.print("    {} (unit {}, {}): '{}'{}", slot.name, slot.unit, slot.uniform, it->second, exists ? "" : "  [MISSING]");
          if (!exists) {
            ctx.print("  warning: texture '{}' resolves to '{}' which does not exist", it->second, abs.string());
            ++warning_count;
          }
        }

        for (const auto& [hash, path] : mat.texture_paths) {
          if (path.empty()) {
            continue;
          }
          const bool known = std::ranges::find(layout.texture_slots, hash, &material_layout::texture_slot::name_hash) != layout.texture_slots.end();
          if (!known) {
            const auto name_it = mat.texture_paths.find(hash);
            ctx.print("  warning: texture slot '{}' is not declared by the pipeline's layout; ignored",
                      name_it != mat.texture_paths.end() ? name_it->second : std::format("{:#018x}", hash));
            ++warning_count;
          }
        }

        if (warning_count == 0) {
          return tool_result::ok(std::format("ok: '{}' packs cleanly against '{}'", mat.name, pipeline_path.filename().string()));
        }
        return tool_result::ok(std::format("{} warning(s) — the material still renders; mismatched params fall back to layout defaults", warning_count));
      }

    }  // namespace

    std::string_view material_tool::usage() const {
      return kUsage;
    }

    tool_result material_tool::execute(tool_context& ctx, std::span<const std::string> args) {
      PROFILE_SECTION("material_tool::execute");
      ensure_cli_runtime();

      parsed_arguments parsed = parse_arguments(args);
      if (parsed.error.has_value()) {
        return tool_result::error(std::format("{}\n{}", *parsed.error, kUsage));
      }

      if (!parsed.input.is_absolute()) {
        parsed.input = ctx.working_directory / parsed.input;
      }
      if (parsed.pipeline.has_value() && !parsed.pipeline->is_absolute()) {
        parsed.pipeline = ctx.working_directory / *parsed.pipeline;
      }

      /// ".omat" is asset.hpp's MATERIAL extension row; no named constant exists yet
      if (parsed.input.extension().string() != ".omat") {
        return tool_result::error(std::format("'{}' expects an .omat input, got '{}'\n{}", parsed.command, parsed.input.string(), kUsage));
      }
      if (!std::filesystem::exists(parsed.input)) {
        return tool_result::error(std::format("input file '{}' does not exist", parsed.input.string()));
      }

      if (parsed.command == "info") {
        if (parsed.pipeline.has_value()) {
          return tool_result::error(std::format("'info' takes no --pipeline\n{}", kUsage));
        }
        return info(ctx, parsed.input);
      }
      if (parsed.command == "check") {
        if (!parsed.pipeline.has_value()) {
          return tool_result::error(std::format("'check' needs -p/--pipeline\n{}", kUsage));
        }
        /// no extension gate on the pipeline: the asset table reserves .orpl but the live
        ///  pipelines today are plain .toml (resources/editor-assets/)
        if (!std::filesystem::exists(*parsed.pipeline)) {
          return tool_result::error(std::format("pipeline file '{}' does not exist", parsed.pipeline->string()));
        }
        return check(ctx, parsed.input, *parsed.pipeline);
      }
      return tool_result::error(std::format("unknown command '{}'\n{}", parsed.command, kUsage));
    }

  }  // namespace cli
}  // namespace other