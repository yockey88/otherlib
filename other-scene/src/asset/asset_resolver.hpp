/**
 * \file asset/asset_resolver.hpp
 **/
#ifndef OTHER_SCENE_ASSET_ASSET_RESOLVER_HPP
#define OTHER_SCENE_ASSET_ASSET_RESOLVER_HPP

#include "asset/asset.hpp"

namespace other {

  struct dependency_declaration {
    std::string virtual_path;
    asset::type type = asset::EMPTY;
    bool baked = false;
  };

  using manifest_parser_fn = ostd::vector<dependency_declaration> (*)(const filepath& manifest_path);
  using manifest_builder_fn = opt<manifest_domain> (*)(const filepath& manifest_path);
  struct manifest_parser_table {
    static std::array<manifest_parser_fn, kNumAssetTypes> parsers;
  };
  struct manifest_domain_table {
    static std::array<manifest_builder_fn, kNumAssetTypes> builders;
  };

  struct dependency_snapshot {
    struct node {
      natural_t stable_id = 0;
      asset::type type = asset::type::EMPTY;
      std::string virtual_path;

      /// xxh3 of the manifest bytes that produced this node's edges
      natural_t manifest_hash = 0;
    };

    ostd::vector<node> nodes;                           /// index = node slot
    ostd::vector<std::pair<uint32_t, uint32_t>> edges;  /// (parent_slot, child_slot)
    ostd::vector<ostd::vector<uint32_t>> reverse;       /// child_slot -> parent slots
    ostd::vector<ostd::vector<uint32_t>> topo_layers;   /// layer 0 = leaves; load order

    const node* find(natural_t stable_id) const;
  };

  struct resolve_delta {
    ostd::vector<natural_t> added;
    ostd::vector<natural_t> removed;
    ostd::vector<natural_t> modified;
    ostd::vector<natural_t> affected_parents;  /// transitive closure over 'reverse', in topo order

    bool empty() const;
    std::string to_string() const;
  };

  class asset_resolver {
   public:
    asset_resolver() = default;
    ~asset_resolver() = default;

    dependency_snapshot resolve(std::span<const filepath> roots);
    resolve_delta re_resolve(dependency_snapshot& snap, const filepath& changed);

    std::span<const manifest_domain> manifest_domains() const { return domains; }

   private:
    ostd::vector<manifest_domain> domains;
    ostd::vector<natural_t> root_ids;

    ostd::vector<dependency_declaration> parse_manifest(const dependency_snapshot::node& n);
    natural_t effective_hash_for(const dependency_snapshot::node& n, std::span<const dependency_declaration> decls);
  };

  namespace detail {

    static uint32_t slot_of(const dependency_snapshot& snap, natural_t stable_id) {
      for (uint32_t slot = 0; slot < snap.nodes.size(); ++slot) {
        if (snap.nodes[slot].stable_id == stable_id) return slot;
      }
      OTHER_ASSERT(false, "stable_id {:#x} not in snapshot", stable_id);
      return 0;
    }

  }  // namespace detail
}  // namespace other

#endif  // OTHER_SCENE_ASSET_ASSET_RESOLVER_HPP