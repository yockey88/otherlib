/**
 * \file core/job_graph.cpp
 **/
#include "core/job_graph.hpp"

namespace other {

  ref<job> job_graph::insert(job::descriptor desc, std::function<void()> work) {
    auto handle = make_ref<job>();
    handle->id = allocate_id();
    handle->current_status.store(job::status::PENDING, std::memory_order_release);

    {
      std::lock_guard lck{ graph_mutex };
      job_node node_val{
        .handle = handle,
        .descriptor = std::move(desc),
        .work = std::move(work),
      };

      uint64_t graph_id = work_graph.add_node(std::move(node_val));
      id_pairs.push_back({ .job_id = handle->id, .node_id = graph_id });
    }

    return handle;
  }

  ref<job> job_graph::add_deferred(natural_t trigger, deferred_factory_fn factory) {
    ref<job> placeholder = make_ref<job>();
    placeholder->id = allocate_id();
    placeholder->current_status.store(job::status::PENDING, std::memory_order_release);

    {
      std::lock_guard lck{ graph_mutex };
      deferred_edges.push_back({
        .from_job_id = trigger,
        .factory = std::move(factory),
        .placeholder = placeholder,
      });
    }

    return placeholder;
  }

  void job_graph::add_dependency(natural_t parent, natural_t child) {
    std::lock_guard lck{ graph_mutex };
  }

  natural_t job_graph::node_id_from_job_id(natural_t job_id) const {
    for (const auto& pair : id_pairs) {
      if (pair.job_id == job_id) {
        return pair.node_id;
      }
    }
    return 0;
  }

  natural_t job_graph::job_id_from_node_id(natural_t node_id) const {
    for (const auto& pair : id_pairs) {
      if (pair.node_id == node_id) {
        return pair.job_id;
      }
    }
    return 0;
  }

  job_graph::job_node* job_graph::find_node(natural_t id) {
    return work_graph.ptr_to_node_value(id);
  }

}  // namespace other
