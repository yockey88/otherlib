/**
 * \file core/job_graph.cpp
 **/
#include "core/job_graph.hpp"

namespace other {

  job_graph::job_node* job_graph::get_node(natural_t id) {
    std::lock_guard lck{ graph_mutex };
    auto* n = find_node(node_id_from_job_id(id));
    OTHER_ASSERT(n != nullptr, "Job with ID {} not found in job graph.", id);
    return n;
  }

  ref<job> job_graph::insert(job::descriptor desc, work_fn work) {
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
    OTHER_ASSERT(parent != child, "A job cannot depend on itself. Job ID: {}", parent);
    OTHER_ASSERT(parent != 0 && child != 0, "Job IDs must be non-zero. Parent ID: {}, Child ID: {}", parent, child);

    std::lock_guard lck{ graph_mutex };

    natural_t parent_node_id = node_id_from_job_id(parent);
    natural_t child_node_id = node_id_from_job_id(child);
    if (parent_node_id == 0) {
      CORE_LOG_ERROR("Parent job with ID {} not found in job graph.", parent);
      return;
    }
    if (child_node_id == 0) {
      CORE_LOG_ERROR("Child job with ID {} not found in job graph.", child);
      return;
    }

    job_node* child_node = find_node(child_node_id);
    job_node* parent_node = find_node(parent_node_id);
    if (parent_node == nullptr) {
      CORE_LOG_ERROR("Parent job with ID {} not found in job graph.", parent);
      return;
    }
    if (child_node == nullptr) {
      CORE_LOG_ERROR("Child job with ID {} not found in job graph.", child);
      return;
    }

    work_graph.add_edge(parent_node_id, child_node_id);
    child_node->waiting_on++;
    child_node->handle->current_status.store(job::status::WAITING_FOR_DEPENDENCIES, std::memory_order_release);
  }

  std::vector<natural_t> job_graph::collect_ready() {
    std::lock_guard lck{ graph_mutex };

    std::vector<natural_t> ready_jobs;
    work_graph.for_each_node([&ready_jobs](const job_node& node) {
      if (!node.dispatched && node.waiting_on == 0 && !node.handle->done()) {
        ready_jobs.push_back(node.handle->id);
      }
    });
    std::ranges::sort(ready_jobs, [this](natural_t a, natural_t b) {
      job_node* node_a = find_node(node_id_from_job_id(a));
      job_node* node_b = find_node(node_id_from_job_id(b));
      OTHER_ASSERT(node_a != nullptr, "Job with ID {} not found in job graph.", a);
      OTHER_ASSERT(node_b != nullptr, "Job with ID {} not found in job graph.", b);
      return node_a->descriptor.priority > node_b->descriptor.priority;
    });

    return ready_jobs;
  }

  std::vector<natural_t> job_graph::resolve(natural_t job_id, job::status status) {
    job_node* completed_node = find_node(node_id_from_job_id(job_id));
    if (completed_node == nullptr) {
      return {};
    }

    std::lock_guard lck{ graph_mutex };
    std::vector<natural_t> newly_ready;

    completed_node->handle->current_status.store(status, std::memory_order_release);
    for (const natural_t child_node_id : work_graph.get_neighbors(node_id_from_job_id(job_id))) {
      job_node* child_node = find_node(child_node_id);
      OTHER_ASSERT(child_node != nullptr, "Child node with ID {} not found in job graph.", child_node_id);

      if (status == job::status::FAILED || status == job::status::CANCELLED) {
        child_node->handle->current_status.store(job::status::CANCELLED, std::memory_order_release);
        continue;
      }

      child_node->waiting_on--;
      if (child_node->waiting_on == 0 && !child_node->dispatched) {
        newly_ready.push_back(child_node->handle->id);
      }
    }

    auto removed = std::ranges::remove_if(deferred_edges, [&](deferred_edge& edge) {
      if (edge.from_job_id != job_id) {
        return false;
      }

      if (status == job::status::FAILED || status == job::status::CANCELLED) {
        edge.placeholder->current_status.store(job::status::CANCELLED, std::memory_order_release);
        return true;
      }

      auto [desc, work] = edge.factory(status);
      job_node node_val{
        .handle = edge.placeholder,
        .descriptor = std::move(desc),
        .work = std::move(work),
      };

      natural_t node_id = work_graph.add_node(std::move(node_val));
      natural_t job_id = edge.placeholder->id;
      id_pairs.push_back({ .job_id = job_id, .node_id = node_id });

      newly_ready.push_back(job_id);
      return true;
    });
    deferred_edges.erase(removed.begin(), removed.end());

    work_graph.remove_neighbors(node_id_from_job_id(job_id));

    return newly_ready;
  }

  void job_graph::mark_dispatched(natural_t job_id) {
    std::lock_guard lck{ graph_mutex };
    if (auto* n = find_node(node_id_from_job_id(job_id)); n != nullptr) {
      n->dispatched = true;
      n->handle->current_status.store(job::status::QUEUED, std::memory_order_release);
    }
  }

  void job_graph::cancel(natural_t job_id) {
    resolve(job_id, job::status::CANCELLED);
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

  bool job_graph::empty() const {
    std::lock_guard lck{ graph_mutex };
    return work_graph.empty();
  }

  bool job_graph::size() const {
    std::lock_guard lck{ graph_mutex };
    return work_graph.size();
  }

  job_graph::job_node* job_graph::find_node(natural_t id) {
    return work_graph.ptr_to_node_value(id);
  }

}  // namespace other
