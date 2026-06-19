/**
 * \file core/job_graph.hpp
 **/
#ifndef OTHER_CORE_CORE_JOB_GRAPH_HPP
#define OTHER_CORE_CORE_JOB_GRAPH_HPP

#include "core/job.hpp"

#include "data-structures/graph.hpp"

namespace other {

  class OTHER_CLASS job_graph {
   public:
    using deferred_work = std::pair<job::descriptor, std::function<void()>>;
    using work_fn = std::function<void()>;
    using deferred_factory_fn = std::function<deferred_work(job::status)>;
    struct job_node {
      ref<job> handle = nullptr;
      job::descriptor descriptor;
      /// invalid after dispatch
      work_fn work;

      uint32_t waiting_on = 0;
      bool dispatched = false;
    };
    job_graph() = default;
    ~job_graph() = default;

    job_node* get_node(natural_t id);

    ref<job> insert(job::descriptor desc, work_fn work);
    ref<job> add_deferred(natural_t trigger, job::descriptor desc, work_fn work);
    void add_dependency(natural_t parent, natural_t child);

    std::vector<natural_t> collect_ready();
    std::vector<natural_t> resolve(natural_t job_id, job::status status);
    void mark_dispatched(natural_t job_id);
    void cancel(natural_t job_id);
    void remove(natural_t job_id);

    natural_t node_id_from_job_id(natural_t job_id) const;
    natural_t job_id_from_node_id(natural_t node_id) const;

    bool empty() const;
    bool size() const;

   private:
    struct id_pair {
      natural_t job_id;
      natural_t node_id;
    };
    // struct deferred_edge {
    //   natural_t from_job_id;
    //   deferred_factory_fn factory;
    //   ref<job> placeholder;
    // };
    mutable std::mutex graph_mutex;

    graph<job_node> work_graph;
    std::vector<id_pair> id_pairs;
    // std::vector<deferred_edge> deferred_edges;

    natural_t next_job_id_ = 1;
    natural_t allocate_id() { return next_job_id_++; }

    job_node* find_node(natural_t id);
  };

}  // namespace other

#endif  // OTHER_CORE_CORE_JOB_GRAPH_HPP