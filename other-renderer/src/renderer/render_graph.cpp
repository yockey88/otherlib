/**
 * \file renderer/render_graph.cpp
 **/
#include "renderer/render_graph.hpp"

#include "core/defines.hpp"
#include "core/profiler.hpp"

#include "gpu_resource/renderer_resource.hpp"
#include "gpu_resource/texture.hpp"
#include "renderer/renderer.hpp"

namespace other {

  render_graph::pass_builder& render_graph::pass_builder::set_clear_color(const glm::vec4& clear_color) {
    pass.clear_color = clear_color;
    return *this;
  }

  render_graph::pass_builder& render_graph::pass_builder::texture_resource(resource_handle handle, const std::string_view uname, natural_t slot, framebuffer::attachment_type type, access_flags flags, uint32_t mip_level) {
    auto [itr, success] = pass.texture_resources.insert({ get_next_texture_id(), { .uniform_name = std::string(uname), .type = type, .slot = slot, .flags = flags, .handle = handle, .mip_level = mip_level } });
    if (!success) {
      CORE_LOG_ERROR("Could not add texture resource [{}]. texture resources already bound at {}", handle, slot);
    }
    return *this;
  }

  render_graph::pass_builder& render_graph::pass_builder::buffer_resource(resource_handle handle, uint32_t binding, access_flags flags) {
    auto [itr, success] = pass.buffer_resources.insert({ get_next_buffer_id(), { .binding_point = binding, .flags = flags, .handle = handle } });
    if (!success) {
      CORE_LOG_ERROR("Could not add buffer resource [{}]. Buffer resources already bound at {}", handle, binding ? std::to_string(binding) : "0");
    }
    return *this;
  }

  render_graph::pass_builder& render_graph::pass_builder::execution_callback(pass_executor&& executor, void* user_data) {
    auto [itr, inserted] = graph.executors.insert({ pass.id, std::move(executor) });
    if (!inserted) {
      CORE_LOG_ERROR("Already added execution callback to pass [{}]. Cannot rebind", pass.id);
    } else {
      pass.user_data = user_data;
    }
    return *this;
  }

  render_graph::pass_builder& render_graph::pass_builder::depends_on(const std::string_view pass_name) {
    pass.depends_on.push_back(std::string(pass_name));
    return *this;
  }

  render_graph::pass_builder& render_graph::pass_builder::add_uniform(const std::string_view name, const value& val) {
    natural_t name_hash = FNV(name);
    if (pass.uniforms.contains(name_hash)) {
      CORE_LOG_ERROR("Uniform with name '{}' already exists for pass '{}'. Cannot add uniform.", name, pass.name);
    } else {
      pass.uniforms[name_hash] = {
        .name = std::string(name),
        .val = val
      };
    };
    return *this;
  }

  render_graph& render_graph::pass_builder::end_pass() {
    PROFILE_SECTION("render_graph::pass_builder::end_pass");
    if (pass.framebuffer_handle.has_value() && pass.pass_type == render_pass::RENDER_PASS) {
      auto& fb = graph.renderer_ptr->get_resource<framebuffer>(*pass.framebuffer_handle);

      fb.set_size(pass.size.x, pass.size.y)
        .set_samples(pass.samples)
        .set_clear_color({ 0.1f, 0.1f, 0.1f, 1.f });
      for (const auto& [_, texture] : pass.texture_resources) {
        if (texture.flags & WRITE) {
          fb.add_attachment(texture.handle, texture.type, texture.mip_level);
        }
      }
      fb.finalize_framebuffer();
    }

    // Process depends_on to add edges in the graph
    pass.next_texture_id = curr_texture_id;
    pass.next_buffer_id = curr_buffer_id;

    return graph;
  }

  render_graph::~render_graph() {
    PROFILE_SECTION("render_graph::~render_graph");
    for (auto& [_, pass] : passes) {
      if (pass.pass.framebuffer_handle.has_value()) {
        renderer_ptr->destroy_resource(*pass.pass.framebuffer_handle);
      }
    }
  }

  render_graph& render_graph::start_pipeline() {
    return *this;
  }

  void render_graph::end_pipeline() {
    build_graph();
  }

  render_graph::pass_builder render_graph::start_pass(const std::string_view name, opt<resource_handle> shader_handle, render_pass::type rptype, const glm::vec2& size, bool create_framebuffer, uint32_t samples) {
    PROFILE_SECTION("render_graph::start_pass");
    CORE_LOG_DEBUG("Starting pass [{}] with shader [{}] and size [{}, {}]", name, shader_handle.has_value() ? *shader_handle : resource_handle{}, size.x, size.y);
    pass& pass_data = create_pass(rptype);
    render_pass& rp = pass_data.pass;

    auto* backend = renderer_ptr->rendering();
    OTHER_ASSERT(backend != nullptr, "Rendering Backend is null!");

    auto& api = backend->api();
    OTHER_ASSERT(api != nullptr, "Rendering API is null!");

    if (create_framebuffer) {
      rp.framebuffer_handle = api->create_resource(name, FRAMEBUFFER);
    } else {
      rp.framebuffer_handle = std::nullopt;
    }

    rp.shader_handle = shader_handle;
    rp.name = name;
    rp.size = size;
    rp.samples = samples;
    return pass_builder(*this, rp);
  }

  void render_graph::replace_texture_resource(resource_handle old_handle, resource_handle new_handle) {
    PROFILE_SECTION("render_graph::replace_texture_resource");
    for (auto& [_, pass] : passes) {
      for (auto& [_, texture] : pass.pass.texture_resources) {
        if (texture.handle == old_handle) {
          texture.handle = new_handle;
        }
      }
    }

    for (auto& n : pass_graph.nodes) {
      for (auto& [_, texture] : n.second.input_textures) {
        if (texture.handle == old_handle) {
          texture.handle = new_handle;
        }
      }
      for (auto& [_, texture] : n.second.output_textures) {
        if (texture.handle == old_handle) {
          texture.handle = new_handle;
        }
      }
    }
  }

  void render_graph::replace_buffer_resource(resource_handle old_handle, resource_handle new_handle) {
    PROFILE_SECTION("render_graph::replace_buffer_resource");
    for (auto& [_, pass] : passes) {
      for (auto& [_, buffer] : pass.pass.buffer_resources) {
        if (buffer.handle == old_handle) {
          buffer.handle = new_handle;
        }
      }
    }

    for (auto& n : pass_graph.nodes) {
      for (auto& [_, buffer] : n.second.input_buffers) {
        if (buffer.handle == old_handle) {
          buffer.handle = new_handle;
        }
      }
      for (auto& [_, buffer] : n.second.output_buffers) {
        if (buffer.handle == old_handle) {
          buffer.handle = new_handle;
        }
      }
    }
  }

  void render_graph::replace_pass_shader(resource_handle old_handle, resource_handle new_handle) {
    for (auto& [_, pass] : passes) {
      if (pass.pass.shader_handle.has_value() && *pass.pass.shader_handle == old_handle) {
        pass.pass.shader_handle = new_handle;
      }
    }
  }

  render_graph::pass& render_graph::create_pass(render_pass::type rptype) {
    natural_t id = get_next_pass_id();
    auto [itr, inserted] = passes.insert({ id, { .type = rptype, .pass = render_pass{ .pass_type = rptype, .id = id } } });
    OTHER_ASSERT(inserted, "Failed to insert render pass with ID [{}].", id);
    return itr->second;
  }

  void render_graph::build_graph() {
    PROFILE_SECTION("render_graph::build_graph");
    bool all_passes_have_executors = true;
    for (auto& [id, pass] : passes) {
      auto itr = executors.find(id);
      if (itr == executors.end()) {
        CORE_LOG_ERROR("No executor found for pass ID: {}", id);
        all_passes_have_executors = false;
        break;
      }
    }
    if (!all_passes_have_executors) {
      CORE_LOG_ERROR("Not all passes have executors, cannot build graph.");
      return;
    }

    {
      PROFILE_SECTION("render_graph::build_graph--nodes_and_edges");
      ostd::vector<frame_node> nodes;
      nodes.reserve(passes.size());

      for (auto& [id, pass] : passes) {
        auto& n = nodes.emplace_back() = frame_node{
          .id = pass.pass.id,
          .pass = &pass.pass,
        };

        for (const auto& [id, texture] : pass.pass.texture_resources) {
          if (texture.flags & READ || texture.flags & SAMPLE) {
            n.input_textures.insert({ id, texture });
          }
          if (texture.flags & WRITE) {
            n.output_textures.insert({ id, texture });
          }
        }

        for (const auto& [id, buffer] : pass.pass.buffer_resources) {
          if (buffer.flags & READ) {
            n.input_buffers.insert({ id, buffer });
          }
          if (buffer.flags & WRITE) {
            n.output_buffers.insert({ id, buffer });
          }
        }
      }

      /// list of outgoing edges
      ostd::map<natural_t, std::set<natural_t>> edges;
      for (const auto& n1 : nodes) {
        auto& e1 = edges[n1.id];

        for (const auto& n2 : nodes) {
          if (n1 == n2) {
            continue;
          }
          auto& e2 = edges[n2.id];

          for (const auto& [slot, texture] : n1.output_textures) {
            if (auto itr = std::ranges::find_if(n2.input_textures, [&](const auto pair) -> bool { return pair.second.handle == texture.handle; });
                itr != n2.input_textures.end() &&  // if n2 reads from a texture that n1 writes to
                !e2.contains(n1.id)) {             // and there is not already a backwards edge from n2 to n1
              e1.insert(n2.id);
            }
          }
          for (const auto& [slot, texture] : n1.output_buffers) {
            if (auto itr = std::ranges::find_if(n2.input_buffers, [&](const auto pair) -> bool { return pair.second.handle == texture.handle; });
                itr != n2.input_buffers.end() &&  // if n2 reads from a buffer that n1 writes to
                !e2.contains(n1.id)) {            // and there is not already a backwards edge from n2 to n1
              e1.insert(n2.id);
            }
          }
        }
      }

      for (const auto& n : nodes) {
        for (const auto& depends_on_str : n.pass->depends_on) {
          auto itr = std::ranges::find_if(nodes, [&](const frame_node& node) -> bool { return node.pass->name == depends_on_str; });
          if (itr == nodes.end()) {
            continue;
          }

          auto& e = edges[itr->id];
          if (!e.contains(n.id)) {
            e.insert(n.id);
          }
        }
      }

      for (natural_t i = 0; i < nodes.size(); ++i) {
        const auto& n = nodes[i];
        pass_graph.nodes.insert({ n.id, n });
      }
      pass_graph.edges = std::move(edges);
    }

    topological_sort = get_topological_sort(pass_graph);
    if (topological_sort.empty() || (topological_sort.size() == 1 && topological_sort[0] == static_cast<natural_t>(-1))) {
      if (topological_sort.empty()) {
        CORE_LOG_ERROR("Render graph is not valid, cannot execute.");
      } else {
        CORE_LOG_WARN("Render graph is empty. An empty graph is valid, but if this is unexpected, please check your render passes.");
      }
      output_texture_handle = std::nullopt;
      graph_valid = true;
      dump_pass_graph(pass_graph);
      return;
    }

    graph_valid = true;

    std::stringstream ss;
    ss << "Render Pass Order:";
    if (topological_sort.empty() || (topological_sort.size() == 1 && topological_sort[0] == static_cast<natural_t>(-1))) {
      ss << " (empty graph)";
    } else {
      ss << " ";
      for (const auto& pass_id : topological_sort) {
        auto itr = pass_graph.nodes.find(pass_id);
        if (itr != pass_graph.nodes.end()) {
          ss << itr->second.pass->name << " ";
        }
      }
    }
    CORE_LOG_DEBUG("{}", ss.str());
  }

  /*
L <- Empty list that will contain the sorted elements
S <- Set of all nodes with no incoming edge

while S is not empty do
    remove a node n from S
    add n to L
    for each node m with an edge e from n to m do
        remove edge e from the graph
        if m has no other incoming edges then
            insert m into S

if graph has edges then
    return error   (graph has at least one cycle)
else
    return L   (a topologically sorted order)
  */

  ostd::vector<natural_t> render_graph::get_topological_sort(const graph& g) {
    PROFILE_SECTION("render_graph::get_topological_sort");
    /// if graph is empty (i.e. no passes), return a vector with -1 to signal that this
    ///   is a valid but empty graph (the -1 is to differentiate from an invalid graph which returns {})
    if (g.nodes.empty()) {
      return { static_cast<natural_t>(-1) };
    }

    ostd::map<natural_t, uint32_t> in_degree;
    for (const auto& [id, node] : g.nodes) {
      in_degree[id] = 0;
    }
    for (const auto& [id, node1] : g.nodes) {
      for (const auto& neighbor_id : g.edges.at(id)) {
        OTHER_ASSERT(in_degree.contains(neighbor_id), "In-degree map does not contain neighbor id {}", neighbor_id);
        in_degree[neighbor_id]++;
      }
    }

    std::set<natural_t> ready_nodes;  //< S
    for (const auto& [id, deg] : in_degree) {
      if (deg == 0) {
        ready_nodes.insert(id);
      }
    }

    ostd::vector<natural_t> sorted;  //< L
    sorted.reserve(g.nodes.size());
    while (!ready_nodes.empty()) {
      natural_t n = *ready_nodes.begin();  //< remove n from S
      ready_nodes.erase(ready_nodes.begin());
      sorted.push_back(n);  //< add n to L

      for (const auto& nbr : g.edges.at(n)) {
        --in_degree[nbr];  //< remove edge e from the graph
        if (in_degree[nbr] == 0) {
          ready_nodes.insert(nbr);  //< insert into S
        }
      }
    }

    if (sorted.size() != g.nodes.size()) {
      log_topo_sort_error(g, in_degree);
      return {};
    }
    return sorted;
  }

  void render_graph::dump_pass_graph(const graph& g) {
    std::stringstream ss;
    ss << std::format("Pass Graph:\n");
    for (const auto& [id, node] : g.nodes) {
      ss << std::format("Node {} {{ ", id, node.pass->name);
      if (g.edges.contains(id)) {
        for (const auto& neighbor_id : g.edges.at(id)) {
          ss << std::format(" -> {}", neighbor_id);
        }
      }
      ss << " } ";
      ss << std::format("('{}')\n", node.pass->name);
    }
    CORE_LOG_INFO("{}", ss.str());
  }

  void render_graph::log_topo_sort_error(const graph& g, const ostd::map<natural_t, uint32_t>& remaining_in_degrees) {
    CORE_LOG_ERROR("Topological sort error: graph has cycles.");
    for (const auto& [id, deg] : remaining_in_degrees) {
      if (deg > 0) {
        auto pass_itr = passes.find(id);
        OTHER_ASSERT(pass_itr != passes.end(), "Node {} does not correspond to a valid pass.", id);
        CORE_LOG_ERROR(" - '{}' (ID {}) has {} remaining incoming edges.", pass_itr->second.pass.name, id, deg);
      }
    }
  }

}  // namespace other