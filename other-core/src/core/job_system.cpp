/**
 * \file core/job_system.cpp
 **/
#include "core/job_system.hpp"

#include "core/config_table.hpp"

namespace other {

  void job_system::initialize(const config_table& cfg) {
    config.worker_count = cfg.get_value<uint32_t>("worker_count", std::max(2u, std::thread::hardware_concurrency() - 1));
    pool = make_scope<asio::thread_pool>(config.worker_count);
    OTHER_ASSERT(pool != nullptr, "Failed to create thread pool for job system.");
  }

  void job_system::poll() {
    std::vector<completion_record> completions;
    {
      std::lock_guard lck{ completion_mutex };
      std::swap(completions, pending_completions);
    }

    for (const auto& [id, status] : completions) {
      auto newly_ready = jobs.resolve(id, status);

      for (natural_t node : newly_ready) {
        dispatch_node(node);
      }
    }

    for (auto it = live_coroutines.begin(); it != live_coroutines.end();) {
      it->handle();
      if (it->handle.coro_handle.done()) {
        it->handle.coro_handle.destroy();
        it = live_coroutines.erase(it);
      } else {
        ++it;
      }
    }
  }

  void job_system::shutdown() {
    for (auto& c : live_coroutines) {
      c.handle.coro_handle.destroy();
    }
    live_coroutines.clear();

    OTHER_ASSERT(pool != nullptr, "Thread pool for job system is not initialized.");
    pool->join();
    pool = nullptr;
  }

  ref<job> job_system::submit(job::descriptor desc, job_graph::work_fn work) {
    auto j = jobs.insert(desc, work);
    dispatch_ready();
    return j;
  }

  ref<job> job_system::submit(job::descriptor desc, job_graph::work_fn work, std::span<const natural_t> dependencies) {
    auto handle = jobs.insert(std::move(desc), std::move(work));
    for (auto dep : dependencies) {
      jobs.add_dependency(jobs.node_id_from_job_id(dep), jobs.node_id_from_job_id(handle->id));
    }
    dispatch_ready();
    return handle;
  }

  ref<job> job_system::submit_deferred(natural_t trigger_id, job_graph::deferred_factory_fn factory) {
    return jobs.add_deferred(trigger_id, std::move(factory));
  }

  void job_system::post_coroutine(task coro) {
    live_coroutines.push_back({ .handle = std::move(coro) });
  }

  void job_system::cancel(natural_t id) {
    jobs.cancel(id);
  }

  void job_system::dispatch_ready() {
    auto ready = jobs.collect_ready();
    for (natural_t job_id : ready) {
      dispatch_node(job_id);
    }
  }

  void job_system::dispatch_node(natural_t id) {
    job_graph::job_node* node = jobs.get_node(id);
    OTHER_ASSERT(node != nullptr, "Job node with ID {} not found for dispatch.", id);

    jobs.mark_dispatched(id);

    auto w = node->work;
    auto aff = node->descriptor.thread_affinity;

    auto wrapped = [this, w = std::move(w), id]() {
      job::status status = job::status::COMPLETED;
      try {
        w();
      } catch (const std::exception& e) {
        CORE_LOG_ERROR("Exception in job with ID {}: {}", id, e.what());
        status = job::status::FAILED;
      } catch (...) {
        CORE_LOG_ERROR("Unknown exception in job with ID {}.", id);
        status = job::status::FAILED;
      }
      on_job_complete(id, status);
    };

    switch (aff) {
      case job::affinity::MAIN_THREAD:
        asio::post(main_io_context, std::move(wrapped));
        break;

      case job::affinity::WORKER_THREAD:
      /// \todo choose smart, don't always have to post to thread pool, we can be smarter
      case job::affinity::ANY_THREAD:
        asio::post(*pool, std::move(wrapped));
        break;
      default:
        OTHER_ASSERT(false, "Unknown thread affinity for job with ID {}.", id);
    }
  }

  void job_system::on_job_complete(natural_t id, job::status status) {
    std::lock_guard lck{ completion_mutex };
    pending_completions.push_back({ .id = id, .status = status });
  }

}  // namespace other