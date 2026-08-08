/**
 * \file asset/asset_resolver.cpp
 **/
#include "asset/asset_resolver.hpp"

#include <cstring>
#include <unordered_set>

#include <nlohmann/json.hpp>

#include <tinyxml2/tinyxml2.h>

#include "core/profiler.hpp"
#include "file/filesystem.hpp"
#include "serialization/scene_serializer.hpp"

#include "gpu_resource/material.hpp"

#include "dotnet/csproj_helpers.hpp"

namespace other {
  namespace detail {

    static ostd::vector<uint8_t> read_file_bytes(const filepath& p) {
      std::ifstream file(p, std::ios::binary);
      OTHER_ASSERT(file.is_open(), "cannot read manifest '{}'", p.string());
      return ostd::vector<uint8_t>{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
    }

    static std::string format_cycle(const dependency_snapshot& snap, std::span<const uint32_t> remaining_children) {
      uint32_t start = 0;
      while (start < remaining_children.size() && remaining_children[start] == 0) ++start;
      OTHER_ASSERT(start < remaining_children.size(), "format_cycle called with no cycle");

      ostd::vector<uint32_t> path;
      ostd::vector<bool> on_path(snap.nodes.size(), false);
      uint32_t current = start;
      while (!on_path[current]) {
        on_path[current] = true;
        path.push_back(current);
        for (const auto& [parent, child] : snap.edges) {
          if (parent == current && remaining_children[child] != 0) {
            current = child;
            break;
          }
        }
      }

      std::string out;
      bool in_cycle = false;
      for (const uint32_t slot : path) {
        if (slot == current) {
          in_cycle = true;
        }

        if (!in_cycle) {
          continue;
        }
        out += snap.nodes[slot].virtual_path;
        out += " -> ";
      }
      out += snap.nodes[current].virtual_path;
      return out;
    }

    static void build_reverse_and_layers(dependency_snapshot& snap) {
      PROFILE_SECTION("build_reverse_and_layers");
      const size_t n = snap.nodes.size();
      snap.reverse.assign(n, {});
      ostd::vector<uint32_t> remaining_children(n, 0);
      for (const auto& [parent, child] : snap.edges) {
        snap.reverse[child].push_back(parent);
        ++remaining_children[parent];
      }

      ostd::vector<uint32_t> layer;
      for (uint32_t slot = 0; slot < n; ++slot) {
        if (remaining_children[slot] == 0) {
          layer.push_back(slot);
        }
      }

      size_t placed = 0;
      snap.topo_layers.clear();
      while (!layer.empty()) {
        placed += layer.size();
        ostd::vector<uint32_t> next;
        for (const uint32_t slot : layer) {
          for (const uint32_t parent : snap.reverse[slot]) {
            if (--remaining_children[parent] == 0) {
              next.push_back(parent);
            }
          }
        }

        snap.topo_layers.push_back(std::move(layer));
        layer = std::move(next);
      }

      OTHER_ASSERT(placed == n, "dependency cycle: {}", format_cycle(snap, remaining_children));
    }

    static uint32_t intern(dependency_snapshot& snap, ostd::unordered_map<natural_t, uint32_t>& slots,
                           std::string virtual_path, asset::type type) {
      const natural_t id = stable_id_for(virtual_path);
      if (const auto it = slots.find(id); it != slots.end()) {
        OTHER_ASSERT(snap.nodes[it->second].virtual_path == virtual_path, "stable_id collision: '{}' vs '{}'", snap.nodes[it->second].virtual_path, virtual_path);
        if (snap.nodes[it->second].type == asset::EMPTY) {
          snap.nodes[it->second].type = type;
        }

        return it->second;
      }

      const uint32_t slot = static_cast<uint32_t>(snap.nodes.size());
      snap.nodes.push_back({
        .stable_id = id,
        .type = type,
        .virtual_path = std::move(virtual_path),
      });
      slots.emplace(id, slot);

      return slot;
    }

    static bool delta_bucket_is_parent_seed(natural_t seed, const resolve_delta& delta) {
      return !std::ranges::contains(delta.added, seed) && !std::ranges::contains(delta.modified, seed);
    }

    static void garbage_collect_unreachable(dependency_snapshot& snap, std::span<const natural_t> root_ids) {
      PROFILE_SECTION("garbage_collect_unreachable");
      ostd::vector<bool> live(snap.nodes.size(), false);
      ostd::vector<uint32_t> worklist;
      for (const natural_t root : root_ids) {
        worklist.push_back(slot_of(snap, root));  /// roots are never collected; missing root = fatal
      }
      while (!worklist.empty()) {
        const uint32_t slot = worklist.back();
        worklist.pop_back();
        if (live[slot]) continue;
        live[slot] = true;
        for (const auto& [parent, child] : snap.edges) {
          if (parent == slot && !live[child]) worklist.push_back(child);
        }
      }

      /// compact nodes + remap edges; dead slots simply vanish
      ostd::vector<uint32_t> remap(snap.nodes.size(), UINT32_MAX);
      ostd::vector<dependency_snapshot::node> kept;
      for (uint32_t slot = 0; slot < snap.nodes.size(); ++slot) {
        if (live[slot]) {
          remap[slot] = static_cast<uint32_t>(kept.size());
          kept.push_back(std::move(snap.nodes[slot]));
        }
      }
      snap.nodes = std::move(kept);
      std::erase_if(snap.edges, [&](const auto& e) { return !live[e.first] || !live[e.second]; });
      for (auto& [parent, child] : snap.edges) {
        parent = remap[parent];
        child = remap[child];
      }
    }

    filepath produced_assembly_path(const filepath& csproj) {
      return dir_of(csproj) / "bin" / get_project_build_config_string() / (csproj.stem().string() + ".dll");
    }

    static void apply_produces(dependency_snapshot& snap, ostd::unordered_map<natural_t, uint32_t>& slots,
                               ostd::vector<natural_t>& root_ids, uint32_t producer_slot) {
      const auto produces = asset_resolver_tables::declarations[snap.nodes[producer_slot].type];
      if (produces == nullptr) {
        return;
      }

      opt<dependency_declaration> artifact = produces(absolute_of(snap.nodes[producer_slot].virtual_path));
      if (!artifact.has_value()) {
        return;
      }

      const uint32_t artifact_slot = intern(snap, slots, artifact->virtual_path, artifact->type);
      if (!std::ranges::contains(snap.edges, std::pair{ artifact_slot, producer_slot })) {
        snap.edges.emplace_back(artifact_slot, producer_slot);
      }

      if (!std::ranges::contains(root_ids, snap.nodes[artifact_slot].stable_id)) {
        root_ids.push_back(snap.nodes[artifact_slot].stable_id);
      }
    }

    ostd::vector<dependency_declaration> parse_csproj_manifest(const filepath& manifest_path);
    ostd::vector<dependency_declaration> parse_model_manifest(const filepath& manifest_path);
    ostd::vector<dependency_declaration> parse_scene_manifest(const filepath& manifest_path);
    ostd::vector<dependency_declaration> parse_material_manifest(const filepath& manifest_path);
    ostd::vector<dependency_declaration> empty_parser(const filepath& manifest_path);

    opt<manifest_domain> build_csproj_manifest_domain(const filepath& manifest_path);

    opt<dependency_declaration> get_csproj_produced_assembly(const filepath& path);

  }  // namespace detail

  std::array<manifest_parser_fn, kNumAssetTypes> asset_resolver_tables::parsers = {
    &detail::empty_parser,             // texture
    &detail::parse_model_manifest,     // model-source
    &detail::empty_parser,             // animation
    &detail::parse_csproj_manifest,    // script-project
    &detail::empty_parser,             // script-source
    &detail::empty_parser,             // script-file
    &detail::empty_parser,             // script
    &detail::empty_parser,             // audio
    &detail::parse_scene_manifest,     // scene
    &detail::empty_parser,             // input-map
    &detail::empty_parser,             // rendering-pipeline
    &detail::empty_parser,             // asset-declaration
    &detail::parse_material_manifest,  // material
    &detail::empty_parser              // empty
  };

  std::array<manifest_builder_fn, kNumAssetTypes> asset_resolver_tables::builders = {
    nullptr,                                // texture
    nullptr,                                // model-source
    nullptr,                                // animation
    &detail::build_csproj_manifest_domain,  // script-project
    nullptr,                                // script-source
    nullptr,                                // script-file
    nullptr,                                // script
    nullptr,                                // audio
    nullptr,                                // scene
    nullptr,                                // input-map
    nullptr,                                // rendering-pipeline
    nullptr,                                // asset-declaration
    nullptr,                                // material
    nullptr                                 // empty
  };

  std::array<dependency_declaration_fn, kNumAssetTypes> asset_resolver_tables::declarations = {
    nullptr,                                // texture
    nullptr,                                // model-source
    nullptr,                                // animation
    &detail::get_csproj_produced_assembly,  // script-project
    nullptr,                                // script-source
    nullptr,                                // script-file
    nullptr,                                // script
    nullptr,                                // audio
    nullptr,                                // scene
    nullptr,                                // input-map
    nullptr,                                // rendering-pipeline
    nullptr,                                // asset-declaration
    nullptr,                                // material
    nullptr                                 // empty
  };

  const dependency_snapshot::node* dependency_snapshot::find(natural_t stable_id) const {
    const auto it = std::ranges::find(nodes, stable_id, &node::stable_id);
    return it != nodes.end() ? &*it : nullptr;
  }

  bool resolve_delta::empty() const {
    return added.empty() && removed.empty() && modified.empty() && affected_parents.empty();
  }

  std::string resolve_delta::to_string() const {
    const auto join = [](std::span<const natural_t> ids) {
      std::string out;
      for (const natural_t id : ids) {
        out += std::format("{:#x} ", id);
      }
      return out.empty() ?
        std::string{ "-" } :
        out;
    };
    return std::format("added[{}] removed[{}] modified[{}] refresh[{}]", join(added), join(removed), join(modified), join(affected_parents));
  }

  dependency_snapshot asset_resolver::resolve(std::span<const filepath> roots) {
    PROFILE_SECTION("asset_resolver::resolve");

    dependency_snapshot snap;
    ostd::unordered_map<natural_t, uint32_t> slots;

    domains.clear();
    root_ids.clear();

    ostd::vector<uint32_t> worklist;
    for (const filepath& root : roots) {
      const filepath abs = std::filesystem::absolute(root);
      const uint32_t slot = detail::intern(snap, slots, virtualize(abs), asset::get_type_from_extension(abs.extension().string()));
      root_ids.push_back(snap.nodes[slot].stable_id);
      worklist.push_back(slot);
    }

    while (!worklist.empty()) {
      const uint32_t slot = worklist.back();
      worklist.pop_back();

      const dependency_snapshot::node& n = snap.nodes[slot];
      const ostd::vector<dependency_declaration> decls = parse_manifest(n);
      snap.nodes[slot].manifest_hash = effective_hash_for(n, decls);

      for (const dependency_declaration& d : decls) {
        const size_t before = snap.nodes.size();
        const uint32_t child_slot = detail::intern(snap, slots, d.virtual_path, d.type);
        snap.edges.emplace_back(slot, child_slot);
        if (snap.nodes.size() > before) {
          worklist.push_back(child_slot);
        }
      }

      detail::apply_produces(snap, slots, root_ids, slot);
    }

    detail::build_reverse_and_layers(snap);
    return snap;
  }

  resolve_delta asset_resolver::re_resolve(dependency_snapshot& snap, const filepath& changed) {
    PROFILE_SECTION("asset_resolver::re_resolve");
    const filepath abs = std::filesystem::absolute(changed);
    const dependency_snapshot::node* known = snap.find(stable_id_for(virtualize(abs)));

    ostd::vector<natural_t> dirty_owners;
    for (const manifest_domain& d : domains) {
      switch (classify(d, abs)) {
        case domain_hit::INSIDE:
          dirty_owners.push_back(d.owner_stable_id);
          break;

        case domain_hit::EXCLUDED:
          CORE_LOG_TRACE("'{}' excluded from domain {:#x}", abs.string(), d.owner_stable_id);
          break;

        case domain_hit::OUTSIDE:
          break;
      }
    }

    if (known != nullptr) {
      /// the changed node may itself be a manifest (editing the csproj hits neither
      //  a domain nor a parent walk); cheap for leaf types via the effective-hash early-out
      dirty_owners.push_back(known->stable_id);
      for (const uint32_t parent : snap.reverse[detail::slot_of(snap, known->stable_id)]) {
        dirty_owners.push_back(snap.nodes[parent].stable_id);
      }
    }

    std::ranges::sort(dirty_owners);
    const auto dead = std::ranges::unique(dirty_owners);
    dirty_owners.erase(dead.begin(), dead.end());

    if (dirty_owners.empty() && known == nullptr) {
      return {};
    }

    dependency_snapshot next = snap;
    ostd::unordered_map<natural_t, uint32_t> slots;
    for (uint32_t slot = 0; slot < next.nodes.size(); ++slot) {
      slots.emplace(next.nodes[slot].stable_id, slot);
    }

    ostd::vector<uint32_t> worklist;
    {
      PROFILE_SECTION("asset_resolver::re_resolve--reparse-dirty");
      for (const natural_t owner : dirty_owners) {
        const uint32_t slot = detail::slot_of(next, owner);
        const dependency_snapshot::node parent = next.nodes[slot];
        const ostd::vector<dependency_declaration> decls = parse_manifest(parent);
        const natural_t hash = effective_hash_for(parent, decls);
        if (hash == parent.manifest_hash) {
          continue;
        }

        next.nodes[slot].manifest_hash = hash;
        std::erase_if(next.edges, [slot](const auto& e) { return e.first == slot; });
        for (const dependency_declaration& decl : decls) {
          const size_t before = next.nodes.size();
          const uint32_t child = detail::intern(next, slots, decl.virtual_path, decl.type);
          next.edges.emplace_back(slot, child);
          if (next.nodes.size() != before) {
            worklist.push_back(child);
          }
        }

        detail::apply_produces(next, slots, root_ids, slot);
      }
    }

    {
      PROFILE_SECTION("asset_resolver::re_resolve--expand-children");
      while (!worklist.empty()) {
        const uint32_t slot = worklist.back();
        worklist.pop_back();

        const dependency_snapshot::node parent = next.nodes[slot];
        const ostd::vector<dependency_declaration> decls = parse_manifest(parent);
        next.nodes[slot].manifest_hash = effective_hash_for(parent, decls);

        for (const dependency_declaration& decl : decls) {
          const size_t before = next.nodes.size();
          const uint32_t child = detail::intern(next, slots, decl.virtual_path, decl.type);
          next.edges.emplace_back(slot, child);
          if (next.nodes.size() != before) {
            worklist.push_back(child);
          }
        }

        detail::apply_produces(next, slots, root_ids, slot);
      }
    }

    detail::garbage_collect_unreachable(next, root_ids);
    detail::build_reverse_and_layers(next);

    resolve_delta delta;
    {
      PROFILE_SECTION("asset_resolver::re_resolve--compute-delta");
      for (const dependency_snapshot::node& n : next.nodes) {
        if (snap.find(n.stable_id) == nullptr) {
          delta.added.push_back(n.stable_id);
        }
      }
      for (const dependency_snapshot::node& n : snap.nodes) {
        if (next.find(n.stable_id) == nullptr) {
          delta.removed.push_back(n.stable_id);
        }
      }
      if (known != nullptr && next.find(known->stable_id) != nullptr) {
        delta.modified.push_back(known->stable_id);
      }

      std::unordered_set<natural_t> seeds;
      seeds.insert(delta.added.begin(), delta.added.end());
      seeds.insert(delta.modified.begin(), delta.modified.end());
      for (const natural_t removed : delta.removed) {
        for (const uint32_t parent : snap.reverse[detail::slot_of(snap, removed)]) {
          seeds.insert(snap.nodes[parent].stable_id);
        }
      }

      std::unordered_set<uint32_t> affected;
      const auto mark_parents = [&](auto&& self, uint32_t slot) -> void {
        for (const uint32_t parent : next.reverse[slot]) {
          if (affected.insert(parent).second) {
            self(self, parent);
          }
        }
      };

      for (const natural_t seed : seeds) {
        if (next.find(seed) != nullptr) {
          const uint32_t slot = detail::slot_of(next, seed);
          if (detail::delta_bucket_is_parent_seed(seed, delta)) {
            affected.insert(slot);
          }
          mark_parents(mark_parents, slot);
        }
      }

      for (const ostd::vector<uint32_t>& layer : next.topo_layers) {
        for (const uint32_t slot : layer) {
          if (affected.contains(slot) && !std::ranges::contains(delta.modified, next.nodes[slot].stable_id)) {
            delta.affected_parents.push_back(next.nodes[slot].stable_id);
          }
        }
      }
    }

    snap = std::move(next);
    return delta;
  }

  ostd::vector<dependency_declaration> asset_resolver::parse_manifest(const dependency_snapshot::node& n) {
    const manifest_parser_fn parser = asset_resolver_tables::parsers[n.type];
    OTHER_ASSERT(parser != nullptr, "no parser slot for asset type {}", n.type);
    return parser(absolute_of(n.virtual_path));
  }

  natural_t asset_resolver::effective_hash_for(const dependency_snapshot::node& n, std::span<const dependency_declaration> decls) {
    PROFILE_SECTION("asset_resolver::effective_hash_for");
    if (decls.empty() && asset_resolver_tables::builders[n.type] == nullptr) {
      return 0;
    }

    const ostd::vector<uint8_t> bytes = detail::read_file_bytes(absolute_of(n.virtual_path));

    XXH3_state_t state;
    XXH3_64bits_reset(&state);
    XXH3_64bits_update(&state, bytes.data(), bytes.size());
    for (const dependency_declaration& d : decls) {
      XXH3_64bits_update(&state, d.virtual_path.data(), d.virtual_path.size());
      XXH3_64bits_update(&state, &d.type, sizeof(d.type));
    }

    if (const manifest_builder_fn builder = asset_resolver_tables::builders[n.type]; builder != nullptr) {
      if (opt<manifest_domain> domain = builder(absolute_of(n.virtual_path)); domain.has_value()) {
        const natural_t set_hash = domain->set.content_hash();
        XXH3_64bits_update(&state, &set_hash, sizeof(set_hash));

        // replace
        std::erase_if(domains, [&](const manifest_domain& d) { return d.owner_stable_id == domain->owner_stable_id; });
        domains.push_back(std::move(*domain));
      }
    }
    return XXH3_64bits_digest(&state);
  }

  namespace detail {

    glob_set csproj_compile_set(const tinyxml2::XMLDocument& doc, std::string_view dotnet_config) {
      glob_set compile_set;
      if (csproj_property_or(doc, "EnableDefaultCompileItems", true)) {
        compile_set.include("**/*.cs");
        compile_set.exclude({ "bin/**", "obj/**", ".*/**" });
      }
      for_each_item(doc, "Compile", dotnet_config, [&](const cs_xml::item& it) {
        if (it.is_remove()) {
          compile_set.exclude(it.pattern());
        } else {
          compile_set.include(it.pattern());
        }
      });
      return compile_set;
    }

    ostd::vector<dependency_declaration> parse_csproj_manifest(const filepath& csproj) {
      PROFILE_SECTION("parse_csproj_manifest");
      const std::string build_config = get_environment_build_config_string();
      const std::string_view dotnet_config = dotnet_config_for(build_config);

      tinyxml2::XMLDocument doc;
      const tinyxml2::XMLElement& project = xml::load_project_root(doc, csproj);
      validate_supported_csproj_shape(project, csproj);
      warn_on_directory_build_files(csproj);

      ostd::vector<dependency_declaration> out;

      auto* fs = subsystem<file_system>::get();
      OTHER_ASSERT(fs != nullptr, "file_system subsystem is not available in parse_csproj_manifest");

      const glob_set compile_set = csproj_compile_set(doc, dotnet_config);
      for (const resolved_file& f : fs->expand(dir_of(csproj), compile_set)) {
        out.push_back({ f.virtual_path, asset::SCRIPT_FILE, false });
      }

      for_each_item(doc, "ProjectReference", dotnet_config, [&](const cs_xml::item& it) {
        const filepath referenced = resolve_relative(csproj, it.include());
        OTHER_ASSERT(is_under_any_project_mount(referenced), "ProjectReference '{}' escapes the project mounts", referenced.string());
        out.push_back({ virtualize(referenced), asset::SCRIPT_PROJECT, false });
      });

      for_each_item(doc, "Reference", dotnet_config, [&](const cs_xml::item& it) {
        const filepath declared = it.hint_path();
        if (declared.empty()) {
          return;
        }

        const filepath hint = resolve_relative(csproj, declared.string());
        if (is_under_any_project_mount(hint)) {
          out.push_back({ virtualize(hint), asset::SCRIPT_SOURCE, false });
        } else if (is_environment_owned(hint)) {
          CORE_LOG_DEBUG("csproj '{}': environment-owned reference '{}' (not an asset edge)", csproj.string(), hint.string());
        } else {
          CORE_LOG_WARN("csproj '{}': machine-local reference '{}' — unportable, skipped", csproj.string(), hint.string());
        }
      });

      return out;
    }

    ostd::vector<dependency_declaration> parse_model_manifest(const filepath& model_path) {
      PROFILE_SECTION("parse_model_manifest");
      /// gltf (json) texture deps are scannable without assimp; legacy formats (.fbx/.obj/.dae/.3ds)
      //  resolve textures at load time via the material system instead (documented asymmetry)
      const std::string extension = model_path.extension().string();
      if (extension != ".gltf" && extension != ".glb") {
        return {};
      }

      /// malformed model files are data errors, not contracts: declare no edges and let the
      //  model node's own load surface the failure
      std::ifstream file(model_path, std::ios::binary);
      if (!file.is_open()) {
        CORE_LOG_WARN("model manifest '{}' could not be opened; no edges declared", model_path.string());
        return {};
      }
      const std::string bytes{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };

      std::string_view json_text = bytes;
      if (extension == ".glb") {
        /// glb layout: [magic][version][length] then chunk0 [chunkLength][chunkType 'JSON'] + json bytes;
        //  only the json chunk is read — binary chunks never carry dependencies
        constexpr size_t kGlbHeaderSize = 20;
        constexpr uint32_t kGlbMagic = 0x46546C67;       // 'glTF'
        constexpr uint32_t kJsonChunkType = 0x4E4F534A;  // 'JSON'

        uint32_t magic = 0;
        uint32_t chunk_length = 0;
        uint32_t chunk_type = 0;
        if (bytes.size() >= kGlbHeaderSize) {
          std::memcpy(&magic, bytes.data(), sizeof(uint32_t));
          std::memcpy(&chunk_length, bytes.data() + 12, sizeof(uint32_t));
          std::memcpy(&chunk_type, bytes.data() + 16, sizeof(uint32_t));
        }
        if (bytes.size() < kGlbHeaderSize || magic != kGlbMagic || chunk_type != kJsonChunkType || bytes.size() - kGlbHeaderSize < chunk_length) {
          CORE_LOG_WARN("glb '{}' has an invalid or truncated header; no edges declared", model_path.string());
          return {};
        }
        json_text = std::string_view{ bytes }.substr(kGlbHeaderSize, chunk_length);
      }

      const nlohmann::json doc = nlohmann::json::parse(json_text, nullptr, false);
      if (doc.is_discarded() || !doc.is_object()) {
        CORE_LOG_WARN("model manifest '{}' failed to parse; no edges declared", model_path.string());
        return {};
      }

      auto* fs = subsystem<file_system>::get();
      OTHER_ASSERT(fs != nullptr, "file_system subsystem is not available in parse_model_manifest");

      ostd::vector<dependency_declaration> out;

      /// buffer sidecars (.bin) are deliberately not declared: load_asset rejects unknown extensions,
      //  and baked .omdl — not fake asset nodes — is what makes binary payloads first-class
      if (!doc.contains("images") || !doc["images"].is_array()) {
        return out;
      }
      {
        PROFILE_SECTION("parse_model_manifest--image-edges");
        for (const nlohmann::json& image : doc["images"]) {
          if (!image.is_object() || !image.contains("uri") || !image["uri"].is_string()) {
            continue;  // embedded textures reference a bufferView instead of a uri
          }
          const std::string uri = image["uri"].get<std::string>();
          if (uri.empty() || uri.starts_with("data:")) {
            continue;  // embedded payloads are not filesystem edges
          }

          const filepath abs = resolve_relative(std::filesystem::absolute(model_path), uri);
          if (!std::filesystem::exists(abs)) {
            CORE_LOG_WARN("model '{}' references missing image '{}'; edge skipped", model_path.string(), abs.string());
            continue;
          }
          if (!fs->deep_search_for_mount(abs).is_valid()) {
            CORE_LOG_WARN("model '{}' references '{}' outside every mount; edge skipped", model_path.string(), abs.string());
            continue;
          }

          asset::type type = asset::get_type_from_extension(abs.extension().string());
          if (type == asset::EMPTY) {
            type = asset::TEXTURE;
          }
          std::string virtual_path = virtualize(abs);
          if (!std::ranges::contains(out, virtual_path, &dependency_declaration::virtual_path)) {
            out.push_back({ std::move(virtual_path), type, false });
          }
        }
      }
      return out;
    }

    ostd::vector<dependency_declaration> parse_scene_manifest(const filepath& scene_path) {
      PROFILE_SECTION("parse_scene_manifest");
      /// malformed scene files are data errors, not contracts: declare no edges and let
      //  the scene node's own load surface the parse failure
      serialization::scene_parse_result parsed = serialization::load_scene_document(scene_path);
      if (!parsed.success()) {
        CORE_LOG_WARN("scene manifest '{}' failed to parse: {}", scene_path.string(), parsed.error);
        return {};
      }

      auto* fs = subsystem<file_system>::get();
      OTHER_ASSERT(fs != nullptr, "file_system subsystem is not available in parse_scene_manifest");

      ostd::vector<dependency_declaration> out;
      const auto declare = [&](const filepath& abs, asset::type fallback_type) {
        if (!std::filesystem::exists(abs)) {
          CORE_LOG_WARN("scene '{}' references missing asset '{}'; edge skipped", scene_path.string(), abs.string());
          return;
        }
        if (!fs->deep_search_for_mount(abs).is_valid()) {
          CORE_LOG_WARN("scene '{}' references '{}' outside every mount; edge skipped", scene_path.string(), abs.string());
          return;
        }
        /// extension wins, matching how the resolver types its roots
        asset::type type = asset::get_type_from_extension(abs.extension().string());
        if (type == asset::EMPTY) {
          type = fallback_type;
        }
        std::string virtual_path = virtualize(abs);
        if (!std::ranges::contains(out, virtual_path, &dependency_declaration::virtual_path)) {
          out.push_back({ std::move(virtual_path), type, false });
        }
      };

      if (!parsed.document->script.empty()) {
        declare(resolve_relative(std::filesystem::absolute(scene_path), parsed.document->script), asset::SCRIPT_FILE);
      }
      /// component payload refs are load paths, resolved against the working directory —
      //  the same convention codec_services::resolve_asset applies at instantiation
      for (const serialization::component_asset_ref& ref : serialization::collect_scene_asset_refs(*parsed.document)) {
        declare(std::filesystem::absolute(filepath{ ref.path }).lexically_normal(), ref.type);
      }
      return out;
    }

    ostd::vector<dependency_declaration> parse_material_manifest(const filepath& material_path) {
      PROFILE_SECTION("parse_material_manifest");
      /// malformed material files are data errors, not contracts: declare no edges and let
      //  the material node's own load surface the parse failure
      const material_parse_result parsed = parse_material_toml(material_path);
      if (!parsed.success()) {
        CORE_LOG_WARN("material manifest '{}' failed to parse: {}", material_path.string(), parsed.error);
        return {};
      }

      auto* fs = subsystem<file_system>::get();
      OTHER_ASSERT(fs != nullptr, "file_system subsystem is not available in parse_material_manifest");

      ostd::vector<dependency_declaration> out;
      for (const auto& [slot, rel] : parsed.mat->texture_paths) {
        if (rel.empty()) {
          continue;
        }
        const filepath abs = resolve_relative(std::filesystem::absolute(material_path), rel);
        if (!std::filesystem::exists(abs)) {
          CORE_LOG_WARN("material '{}' references missing texture '{}'; edge skipped", material_path.string(), abs.string());
          continue;
        }
        if (!fs->deep_search_for_mount(abs).is_valid()) {
          CORE_LOG_WARN("material '{}' references '{}' outside every mount; edge skipped", material_path.string(), abs.string());
          continue;
        }

        /// extension wins, matching how the resolver types its roots
        asset::type type = asset::get_type_from_extension(abs.extension().string());
        if (type == asset::EMPTY) {
          type = asset::TEXTURE;
        }
        std::string virtual_path = virtualize(abs);
        if (!std::ranges::contains(out, virtual_path, &dependency_declaration::virtual_path)) {
          out.push_back({ std::move(virtual_path), type, false });
        }
      }
      return out;
    }

    ostd::vector<dependency_declaration> empty_parser(const filepath& manifest_path) {
      return {};
    }

    opt<manifest_domain> build_csproj_manifest_domain(const filepath& manifest_path) {
      PROFILE_SECTION("build_csproj_manifest_domain");
      tinyxml2::XMLDocument doc;
      xml::load_project_root(doc, manifest_path);

      const filepath abs = std::filesystem::absolute(manifest_path);
      return manifest_domain{
        .owner_stable_id = stable_id_for(virtualize(abs)),
        .root_abs = dir_of(abs),
        .set = csproj_compile_set(doc, dotnet_config_for(get_environment_build_config_string())),
      };
    }

    opt<dependency_declaration> get_csproj_produced_assembly(const filepath& path) {
      return dependency_declaration{
        .virtual_path = virtualize(produced_assembly_path(path)),
        .type = asset::SCRIPT_SOURCE,
      };
    }

  }  // namespace detail
}  // namespace other
