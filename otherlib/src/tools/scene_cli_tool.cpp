/**
 * \file tools/scene_cli_tool.cpp
 **/
#include "tools/scene_cli_tool.hpp"

#include <filesystem>

#include <spdlog/sinks/stdout_color_sinks.h>

#include "core/logger_sinks.hpp"
#include "memory/arena.hpp"
#include "serialization/scene_serializer.hpp"

#include "tools/material_cli_tool.hpp"
#include "tools/model_cli_tool.hpp"

namespace other {
  namespace cli {

    /// oecli runs without a driver, but the scene document layer constructs engine
    ///  component types whose containers allocate through the arena, and its error
    ///  paths log — so the two memory/log subsystems must be live before any codec
    ///  runs. inside a running environment they already are and this is a no-op.
    void ensure_cli_runtime() {
      if (subsystem<logger>::inert) {
        static config_table cli_config = [] {
          config_table cfg;
          cfg.core_log_level = static_cast<uint32_t>(spdlog::level::warn);
          return cfg;
        }();

        subsystem<logger>::inert = false;
        logger* log = subsystem<logger>::get();
        OTHER_ASSERT(log != nullptr, "failed to initialize the cli logger");
        log->set_config(&cli_config);
        log->create_logger("other-core-log", spdlog::level::warn);

        /// console only at warn — a cli tool must not scatter log files around
        log_sink console_sink = {
          1,
          "console-sink",
          "%^[%l]%$ %v",
          spdlog::level::warn,
          [](const config_table&) -> spdlog::sink_ptr { return std::make_shared<spdlog::sinks::stdout_color_sink_mt>(); },
        };
        std::string loggers[] = { "other-core-log" };
        log->register_sink(loggers, &console_sink);
      }

      if (subsystem<arena>::inert) {
        subsystem<arena>::inert = false;
        OTHER_ASSERT(subsystem<arena>::get() != nullptr, "failed to initialize the cli arena");
      }
    }

    namespace {

      constexpr std::string_view kUsage =
        "usage: oecli scene <command> <input> [options]\n"
        "\n"
        "commands:\n"
        "  compile <input.oscn> [-o <output.oscnb>]    bake a toml scene document to binary\n"
        "  decompile <input.oscnb> [-o <output.oscn>]  recover a toml scene document from binary\n"
        "  info <input.oscn|.oscnb>                    print a scene document summary\n"
        "\n"
        "options:\n"
        "  -o, --out <path>   output path (default: input with the converted extension)";

      struct parsed_arguments {
        std::string command = "";
        filepath input = "";
        opt<filepath> output = std::nullopt;
        opt<std::string> error = std::nullopt;
      };

      parsed_arguments parse_arguments(std::span<const std::string> args) {
        parsed_arguments parsed = {};
        if (args.empty()) {
          parsed.error = "missing command";
          return parsed;
        }
        parsed.command = args[0];

        for (size_t i = 1; i < args.size(); ++i) {
          const std::string& arg = args[i];
          if (arg == "-o" || arg == "--out") {
            if (i + 1 >= args.size()) {
              parsed.error = std::format("'{}' expects a path argument", arg);
              return parsed;
            }
            parsed.output = filepath(args[++i]);
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

      tool_result convert(tool_context& ctx, const parsed_arguments& parsed, const std::string_view expected_extension, const std::string_view output_extension) {
        if (parsed.input.extension().string() != expected_extension) {
          return tool_result::error(std::format("'{}' expects a {} input, got '{}'", parsed.command, expected_extension, parsed.input.string()));
        }

        serialization::scene_parse_result loaded = serialization::load_scene_document(parsed.input);
        if (!loaded.success()) {
          return tool_result::error(loaded.error);
        }
        for (const std::string& warning : loaded.warnings) {
          ctx.print("warning: {}", warning);
        }

        filepath output = parsed.output.value_or(filepath(parsed.input).replace_extension(output_extension));
        if (!serialization::save_scene_document(*loaded.document, output)) {
          return tool_result::error(std::format("failed to write '{}'", output.string()));
        }

        return tool_result::ok(std::format("{} '{}' -> '{}' ({} objects)", parsed.command, parsed.input.string(), output.string(), loaded.document->objects.size()));
      }

      tool_result info(tool_context& ctx, const parsed_arguments& parsed) {
        serialization::scene_parse_result loaded = serialization::load_scene_document(parsed.input);
        if (!loaded.success()) {
          return tool_result::error(loaded.error);
        }
        for (const std::string& warning : loaded.warnings) {
          ctx.print("warning: {}", warning);
        }

        const serialization::scene_document& doc = *loaded.document;
        ctx.print("scene '{}' (schema {})", doc.name, doc.schema_version);
        if (!doc.script.empty()) {
          ctx.print("  script: {}", doc.script);
        }
        ctx.print("  clear-color: [{}, {}, {}, {}]", doc.clear_color.r, doc.clear_color.g, doc.clear_color.b, doc.clear_color.a);
        ctx.print("  objects: {}", doc.objects.size());
        for (const serialization::object_record& record : doc.objects) {
          std::string components = "";
          for (const serialization::component_record& component : record.components) {
            const serialization::component_codec* codec = serialization::find_component_codec(component.key_hash);
            components += std::format("{}{}", components.empty() ? "" : ", ", codec != nullptr ? std::string{ codec->key } : std::format("{:#018x}", component.key_hash));
          }
          std::string tags = "";
          for (const std::string& tag : record.tags) {
            tags += std::format("{}{}", tags.empty() ? "" : ", ", tag);
          }
          ctx.print("    [{}] {}{}{}{}", record.file_id, record.name,
                    record.parent_file_id != 0 ? std::format(" (parent {})", record.parent_file_id) : "",
                    components.empty() ? "" : std::format(" | {}", components),
                    tags.empty() ? "" : std::format(" | tags: {}", tags));
        }
        return tool_result::ok();
      }

    }  // namespace

    std::string_view scene_tool::usage() const {
      return kUsage;
    }

    tool_result scene_tool::execute(tool_context& ctx, std::span<const std::string> args) {
      ensure_cli_runtime();

      parsed_arguments parsed = parse_arguments(args);
      if (parsed.error.has_value()) {
        return tool_result::error(std::format("{}\n{}", *parsed.error, kUsage));
      }

      if (!parsed.input.is_absolute()) {
        parsed.input = ctx.working_directory / parsed.input;
      }
      if (parsed.output.has_value() && !parsed.output->is_absolute()) {
        parsed.output = ctx.working_directory / *parsed.output;
      }
      if (!std::filesystem::exists(parsed.input)) {
        return tool_result::error(std::format("input file '{}' does not exist", parsed.input.string()));
      }

      if (parsed.command == "compile") {
        return convert(ctx, parsed, serialization::kSceneTomlExtension, serialization::kSceneBinaryExtension);
      }
      if (parsed.command == "decompile") {
        return convert(ctx, parsed, serialization::kSceneBinaryExtension, serialization::kSceneTomlExtension);
      }
      if (parsed.command == "info") {
        return info(ctx, parsed);
      }
      return tool_result::error(std::format("unknown command '{}'\n{}", parsed.command, kUsage));
    }

    void register_environment_tools(tool_registry& registry) {
      if (registry.find_tool("scene") == nullptr) {
        registry.add_tool(std::make_unique<scene_tool>());
      }
      if (registry.find_tool("model") == nullptr) {
        registry.add_tool(std::make_unique<model_tool>());
      }
      if (registry.find_tool("material") == nullptr) {
        registry.add_tool(std::make_unique<material_tool>());
      }
    }

  }  // namespace cli
}  // namespace other
