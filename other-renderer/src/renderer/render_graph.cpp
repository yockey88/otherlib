/**
 * \file renderer/render_graph.cpp
 **/
#include "renderer/render_graph.hpp"

#include "core/defines.hpp"

#include "gpu_resource/renderer_resource.hpp"
#include "gpu_resource/texture.hpp"
#include "renderer/renderer.hpp"

namespace other {

#ifdef RG_TESTING

  render_graph::pass_builder& render_graph::pass_builder::set_clear_color(const glm::vec4& clear_color) {
    pass.clear_color = clear_color;
    return *this;
  }

  render_graph::pass_builder& render_graph::pass_builder::texture_resource(resource_handle handle, natural_t slot, framebuffer::attachment_type type, access_flags flags) {
    auto [itr, success] = pass.texture_resources.insert({ slot, { .type = type, .flags = flags, .handle = handle } });
    if (!success) {
      CORE_LOG_ERROR("Could not add texture resource [{}]. texture resources already bound at {}", handle, slot);
    }
    return *this;
  }

  render_graph::pass_builder& render_graph::pass_builder::buffer_resource(resource_handle handle, natural_t slot, access_flags flags) {
    auto [itr, success] = pass.buffer_resources.insert({ slot, { .flags = flags, .handle = handle } });
    if (!success) {
      CORE_LOG_ERROR("Could not add buffer resource [{}]. Buffer resources already bound at {}", handle, slot);
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

  render_graph& render_graph::pass_builder::end_pass() {
    if (pass.framebuffer_handle.has_value()) {
      auto& fb = graph.renderer_ptr->get_resource<framebuffer>(*pass.framebuffer_handle);

      fb.set_size(pass.size.x, pass.size.y)
        .set_clear_color({ 0.1f, 0.1f, 0.1f, 1.f });
      for (const auto& [_, texture] : pass.texture_resources) {
        if ((texture.flags & WRITE) == WRITE) {
          fb.add_attachment(texture.handle, texture.type);
        }
      }
      fb.finalize_framebuffer();
    }

    return graph;
  }

  render_graph::~render_graph() {
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

  render_graph::pass_builder render_graph::start_pass(const std::string_view name, resource_handle shader_handle, render_pass::type rptype, const glm::vec2& size, bool create_framebuffer) {
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
    return pass_builder(*this, rp);
  }

  render_graph::pass& render_graph::create_pass(render_pass::type rptype) {
    natural_t id = get_next_pass_id();
    auto [itr, inserted] = passes.insert({ id, { .type = rptype, .pass = render_pass{ .id = id } } });
    OTHER_ASSERT(inserted, "Failed to insert render pass with ID [{}].", id);
    return itr->second;
  }

  void render_graph::build_graph() {
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

    std::vector<node> nodes;
    nodes.reserve(passes.size());

    for (auto& [id, pass] : passes) {
      auto& n = nodes.emplace_back() = node{
        .id = get_next_node_id(),
        .pass = &pass.pass,
      };

      for (const auto& [slot, texture] : pass.pass.texture_resources) {
        if ((texture.flags & READ) == READ) {
          n.input_textures.insert({ slot, texture });
        }
        if ((texture.flags & WRITE) == WRITE) {
          n.output_textures.insert({ slot, texture });
        }
      }

      for (const auto& [slot, buffer] : pass.pass.buffer_resources) {
        if ((buffer.flags & READ) == READ) {
          n.input_buffers.insert({ slot, buffer });
        }
        if ((buffer.flags & WRITE) == WRITE) {
          n.output_buffers.insert({ slot, buffer });
        }
      }
    }

    std::vector<std::vector<natural_t>> edges;
    edges.resize(nodes.size());
    for (const auto& n1 : nodes) {
      auto& e1 = edges[n1.id];
      for (const auto& n2 : nodes) {
        if (n1 == n2) {
          continue;
        }

        for (const auto& [slot, texture] : n1.output_textures) {
          if (auto itr = std::ranges::find_if(n2.input_textures, [&](const auto pair) -> bool { return pair.second.handle == texture.handle; }); itr != n2.input_textures.end()) {
            e1.push_back(n2.id);
            CORE_LOG_DEBUG("Adding edge from pass {} to pass {} for texture resource {}", n1.pass->id, n2.pass->id, texture.handle);
          }
        }
        for (const auto& [slot, texture] : n1.output_buffers) {
          if (auto itr = std::ranges::find_if(n2.input_buffers, [&](const auto pair) -> bool { return pair.second.handle == texture.handle; }); itr != n2.input_buffers.end()) {
            e1.push_back(n2.id);
            CORE_LOG_DEBUG("Adding edge from pass {} to pass {} for buffer resource {}", n1.pass->id, n2.pass->id, texture.handle);
          }
        }
      }
    }

    pass_graph = graph{};
    for (natural_t i = 0; i < nodes.size(); ++i) {
      const auto& n = nodes[i];
      const auto& e = edges[i];

      pass_graph.nodes.insert({ n.id, n });
      pass_graph.edges.insert({ n.id, std::move(e) });
    }

    topological_sort = get_topological_sort(pass_graph);
    if (topological_sort.empty()) {
      return;
    }
    graph_valid = true;
  }

  /*
  L ← Empty list that will contain the sorted elements
  S ← Set of all nodes with no incoming edge

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

  std::vector<natural_t> render_graph::get_topological_sort(const graph& g) {
    std::vector<natural_t> sorted;
    sorted.reserve(g.nodes.size());

    std::vector<natural_t> in_degree;
    in_degree.resize(g.nodes.size(), 0);

    std::set<natural_t> no_incoming_edges;
    for (const auto& [id, node1] : g.nodes) {
      for (const auto& neighbor_id : g.edges.at(id)) {
        in_degree[neighbor_id]++;
      }
      if (in_degree[id] == 0) {
        no_incoming_edges.insert(id);
      }
    }

    std::map<natural_t, std::set<natural_t>> edges;
    for (const auto& [id, node] : g.nodes) {
      auto [itr, sucess] = edges.insert({ id, std::set<natural_t>(g.edges.at(id).begin(), g.edges.at(id).end()) });
      OTHER_ASSERT(sucess, "Failed to insert edges for node {}", id);
    }

    while (!no_incoming_edges.empty()) {
      natural_t current = *no_incoming_edges.begin();
      no_incoming_edges.erase(no_incoming_edges.begin());
      sorted.push_back(current);

      auto& current_edges = edges[current];
      while (!current_edges.empty()) {
        natural_t neighbor = *current_edges.begin();
        current_edges.erase(current_edges.begin());

        in_degree[neighbor]--;
        if (in_degree[neighbor] == 0) {
          no_incoming_edges.insert(neighbor);
        }
      }
    }

    bool any_cycles = std::ranges::any_of(in_degree, [](natural_t degree) { return degree > 0; });
    if (any_cycles) {
      CORE_LOG_ERROR("Render graph has cycles, cannot execute.");
      return {};
    } else {
      return sorted;
    }
  }

#else
  render_graph::pass_builder& render_graph::pass_builder::use_texture_resource(resource_handle texture_id, uint32_t binding, access_flags flags) {
    OTHER_ASSERT(pass.graph != nullptr, "Render pass graph is null.");
    render_graph& graph = *pass.graph;
    auto itr = graph.texture_bindings.find(pass.get_key());
    if (itr == graph.texture_bindings.find(pass.get_key())) {
      auto [new_itr, inserted] = graph.texture_bindings.insert({ pass.get_key(), {} });
      OTHER_ASSERT(inserted, "");
      itr = new_itr;
    }
    itr->second.push_back({ flags, binding, texture_id });
    return *this;
  }

  render_graph::pass_builder& render_graph::pass_builder::use_buffer_resource(resource_handle buffer_id, uint32_t binding, access_flags flags) {
    OTHER_ASSERT(pass.graph != nullptr, "Render pass graph is null.");
    render_graph& graph = *pass.graph;
    auto itr = graph.buffer_bindings.find(pass.get_key());
    if (itr == graph.buffer_bindings.end()) {
      auto [new_itr, inserted] = graph.buffer_bindings.insert({ pass.get_key(), {} });
      OTHER_ASSERT(inserted, "Failed to insert buffer binding for render pass [{}].", pass.get_key());
      itr = new_itr;
    }
    itr->second.push_back({ flags, binding, buffer_id });
    return *this;
  }

  render_graph& render_graph::pass_builder::end_pass() {
    OTHER_ASSERT(pass.graph != nullptr, "Render pass graph is null.");
    if (pass.size.x == 0 || pass.size.y == 0) {
      /// get window size
      pass.size = { 800, 600 };
    }

    if (pass.shader_handle.id == 0) {
      CORE_LOG_ERROR("Render pass shader handle is invalid, cannot finalize pass.");
    } else {
      pass.graph->current_pass = nullptr;
      /// other finalization steps
    }

    return *pass.graph;
  }

  render_graph& render_graph::begin_frame() {
    return *this;
  }

  void render_graph::end_frame() {
    build_graph();
    validate_graph();
  }

  render_graph::pass_builder render_graph::start_pass(resource_handle framebuffer, resource_handle shader, const glm::ivec2& size) {
    OTHER_ASSERT(current_pass == nullptr, "Cannot start a new render pass while another one is active.");

    render_pass::key id{ framebuffer, shader };
    {
      auto itr = passes.find(id);
      if (itr != passes.end()) {
        CORE_LOG_ERROR("Render pass with ID [{}] already exists.", id);
        if (current_pass == &itr->second) {
          CORE_LOG_ERROR("Cannot create a pass with the same ID as the current pass.");
          return pass_builder(itr->second);
        }
        OTHER_ASSERT(false, "Render pass with the same ID already exists.");
      }
    }

    natural_t pass_id = get_next_pass_id();
    auto [itr, inserted] = passes.insert({ id, render_pass(this, pass_id) });
    OTHER_ASSERT(inserted, "Failed to insert render pass with ID [{}].", id);

    auto& pass = itr->second;
    pass.size = size;

    pass.framebuffer_handle = framebuffer;
    pass.shader_handle = shader;
    pass.user_data = nullptr;

    current_pass = &pass;

    return pass_builder{ pass };
  }

  render_graph::pass_builder render_graph::start_pass(resource_handle shader, const glm::ivec2& size) {
    OTHER_ASSERT(current_pass == nullptr, "Cannot start a new render pass while another one is active.");

    render_pass::key id{ std::nullopt, shader };
    {
      auto itr = passes.find(id);
      if (itr != passes.end()) {
        CORE_LOG_ERROR("Render pass with ID [{}] already exists.", id);
        if (current_pass == &itr->second) {
          CORE_LOG_ERROR("Cannot create a pass with the same ID as the current pass.");
          return pass_builder(itr->second);
        }
        OTHER_ASSERT(false, "Render pass with the same ID already exists.");
      }
    }

    natural_t pass_id = get_next_pass_id();
    auto [itr, inserted] = passes.insert({ id, render_pass(this, pass_id) });
    OTHER_ASSERT(inserted, "Failed to insert render pass with ID [{}].", id);

    auto& pass = itr->second;
    pass.size = size;

    pass.framebuffer_handle = std::nullopt;
    pass.shader_handle = shader;
    pass.user_data = nullptr;

    current_pass = &pass;
    return pass_builder(pass);
  }

  void render_graph::build_graph() {
    std::vector<node> pass_nodes;
    pass_nodes.reserve(passes.size());
    for (const auto& [key, pass] : passes) {
      node& n = pass_nodes.emplace_back();
      n.pass_id = pass.id;

      for (const auto& binding : buffer_bindings[key]) {
        if (binding.flags & access_flags::READ) {
          n.input_buffers.push_back(key);
        } else if (binding.flags & access_flags::WRITE) {
          n.output_buffers.push_back(key);
        }
      }

      for (const auto& binding : texture_bindings[key]) {
        if (binding.flags & access_flags::READ) {
          n.input_textures.push_back(key);
        } else if (binding.flags & access_flags::WRITE) {
          n.output_textures.push_back(key);
        }
      }

      CORE_LOG_INFO("Pass ID: {}, Input Buffers: {}, Input Textures: {}, Output Buffers: {}, Output Textures: {}", n.pass_id, n.input_buffers.size(), n.input_textures.size(), n.output_buffers.size(), n.output_textures.size());
    }

    // /// connect nodes based on input/output relationships
    // for (const auto& node1 : pass_nodes) {
    //   for (const auto& node2 : pass_nodes) {
    //     if (node1.pass_id == node2.pass_id) {
    //       continue;  // Skip self-dependency
    //     }

    //     // Check if node1's output buffers/textures are used by node2's input buffers/textures
    //     for (const auto& output_buffer : node1.output_buffers) {
    //       if (std::ranges::find(node2.input_buffers, output_buffer) != node2.input_buffers.end()) {
    //         pass_dependencies[node1.pass_id].push_back(node2.pass_id);
    //       }
    //     }

    //     for (const auto& output_texture : node1.output_textures) {
    //       if (std::ranges::find(node2.input_textures, output_texture) != node2.input_textures.end()) {
    //         pass_dependencies[node1.pass_id].push_back(node2.pass_id);
    //       }
    //     }
    //   }
    // }

    // /// node with no output buffers/textures is a root node
    // node* root = nullptr;
  }

  void render_graph::validate_graph() {
  }
#endif

}  // namespace other