/**
 * \file core/job_system.cpp
 **/
#include "core/job_system.hpp"

#include <asio/asio.hpp>

#include "core/config_table.hpp"

#include "defines.hpp"

namespace other {

  job_system::job_system(asio::io_context& main_ctx)
      : main_io_context(main_ctx) {}

  job_system::~job_system() = default;

  void job_system::initialize(const config_table& cfg) {
    config.worker_count = cfg.get_value<uint32_t>("application.async.worker_count", std::max(2u, std::thread::hardware_concurrency() - 1));
    pool = make_scope<asio::thread_pool>(config.worker_count);
    OTHER_ASSERT(pool != nullptr, "Failed to create thread pool for job system.");
  }

  void job_system::poll() {
    ostd::vector<completion_record> completions;
    {
      std::lock_guard lck{ completion_mutex };
      std::swap(completions, pending_completions);
    }

    for (const auto& [id, status] : completions) {
      {
        auto* node = jobs.get_node(id);
        OTHER_ASSERT(node != nullptr, "Job node with ID {} not found in job graph on completion.", id);
        OTHER_ASSERT(node->handle != nullptr, "Job handle for job with ID {} is null on completion.", id);
        node->handle->current_status.store(status, std::memory_order_release);
      }

      auto newly_ready = jobs.resolve(id, status);
      CORE_LOG_DEBUG("Jobs ready: {}", newly_ready);
      for (natural_t node : newly_ready) {
        dispatch_node(node);
      }

      jobs.remove(id);
    }

    {
      std::lock_guard lck{ live_coroutines_mutex };
      running_coroutines = true;
      for (auto it = live_coroutines.begin(); it != live_coroutines.end();) {
        it->handle();
        if (it->handle.coro_handle.done()) {
          it->handle.coro_handle.destroy();
          it = live_coroutines.erase(it);
        } else {
          ++it;
        }
      }

      if (running_coroutines && pending_coroutines.size() > 0) {
        while (!pending_coroutines.empty()) {
          live_coroutines.push_back({ .handle = std::move(pending_coroutines.front()) });
          pending_coroutines.pop();
        }
      }
      running_coroutines = false;
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
    OTHER_ASSERT(work != nullptr, "Work function for job '{}' is null.", desc.name);

    for (natural_t dep : dependencies) {
      if (!jobs.get_node(dep)) {
        CORE_LOG_ERROR("Dependency [{}] not found for job '{}'.", dep, desc.name);
        return nullptr;
      }
    }

    auto handle = jobs.insert(std::move(desc), std::move(work));
    for (auto dep : dependencies) {
      jobs.add_dependency(jobs.node_id_from_job_id(dep), jobs.node_id_from_job_id(handle->id));
    }
    CORE_LOG_DEBUG("Submitted job [{}], dependencies: {}", handle->id, dependencies);
    dispatch_ready();
    return handle;
  }

  ref<job> job_system::submit_deferred(natural_t trigger_id, job::descriptor desc, job_graph::work_fn work) {
    CORE_LOG_DEBUG("Submitting deferred job. Trigger: [{}].", trigger_id);
    return jobs.add_deferred(trigger_id, std::move(desc), std::move(work));
  }

  void job_system::post_coroutine(task&& coro) {
    /// this is to not invalidate the iterators of live_coroutines if we post a new coroutine from within a running coroutine
    if (running_coroutines) {
      std::lock_guard lck{ pending_coroutines_mutex };
      pending_coroutines.push(std::move(coro));
    } else {
      std::lock_guard lck{ live_coroutines_mutex };
      live_coroutines.push_back({ .handle = std::move(coro) });
    }
  }

  void job_system::cancel(natural_t id) {
    CORE_LOG_DEBUG("Cancelling Job [{}].", id);
    jobs.cancel(id);
  }

  void job_system::dispatch_ready() {
    auto ready = jobs.collect_ready();
    CORE_LOG_DEBUG("Dispatching ready jobs: {}", ready);
    for (natural_t job_id : ready) {
      dispatch_node(job_id);
    }
  }

  void job_system::dispatch_node(natural_t id) {
    job_graph::job_node* node = jobs.get_node(id);
    if (node == nullptr) {
      CORE_LOG_ERROR("Job node with ID {} not found for dispatch.", id);
      return;
    }

    jobs.mark_dispatched(id);

    auto w = node->work;
    auto aff = node->descriptor.thread_affinity;

    node->handle->current_status.store(job::status::RUNNING, std::memory_order_release);
    auto wrapped = [this, w = std::move(w), id]() {
      job::status final_status = job::status::COMPLETED;
      try {
        w();
      } catch (const std::exception& e) {
        CORE_LOG_ERROR("Exception in job [{}]: {}", id, e.what());
        final_status = job::status::FAILED;
      } catch (...) {
        CORE_LOG_ERROR("Unknown exception in job [{}].", id);
        final_status = job::status::FAILED;
      }
      on_job_complete(id, final_status);
    };

    // clang-format off
    CORE_LOG_DEBUG("Dispatching Job: [{} : {}], priority = {}, affinity = {}", 
                   node->descriptor.name, id, node->descriptor.priority, aff);
    // clang-format on
    switch (aff) {
      case job::affinity::MAIN_THREAD:
        asio::post(main_io_context, std::move(wrapped));
        break;

      // \todo pick the thread to put 'any-thread' jobs on smarter
      case job::affinity::ANY_THREAD:
      case job::affinity::WORKER_THREAD:
        asio::post(*pool, std::move(wrapped));
        break;
      default:
        OTHER_ASSERT(false, "Unknown thread affinity for job with ID {}.", id);
    }
  }

  void job_system::on_job_complete(natural_t id, job::status status) {
    CORE_LOG_DEBUG("Job [{}] completed with status {}.", id, status);
    std::lock_guard lck{ completion_mutex };
    pending_completions.push_back({ .id = id, .status = status });
  }

}  // namespace other