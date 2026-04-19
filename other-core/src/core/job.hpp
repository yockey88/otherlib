/**
 * \file core/job.hpp
 **/
#ifndef OTHER_CORE_CORE_JOB_HPP
#define OTHER_CORE_CORE_JOB_HPP

#include <atomic>
#include <mutex>

#include "core/defines.hpp"
#include "core/ref_counted.hpp"

namespace other {

  class job : public ref_counted {
   public:
    enum priority : uint8_t {
      LOW,
      MEDIUM,
      HIGH,
      CRITICAL,

      NUM_JOB_PRIORITIES,
      INVALID_JOB_PRIORITY = NUM_JOB_PRIORITIES,
    };
    enum status : uint8_t {
      PENDING,
      QUEUED,
      WAITING_FOR_DEPENDENCIES,
      RUNNING,
      COMPLETED,
      FAILED,
      CANCELLED,

      NUM_JOB_STATUSES,
      INVALID_JOB_STATUS = NUM_JOB_STATUSES,
    };
    enum affinity : uint8_t {
      ANY_THREAD,
      MAIN_THREAD,
      WORKER_THREAD,

      NUM_JOB_AFFINITIES,
      INVALID_JOB_AFFINITY = NUM_JOB_AFFINITIES,
    };
    struct descriptor {
      std::string name = "Unnamed Job";
      priority priority = priority::LOW;
      affinity thread_affinity = affinity::ANY_THREAD;
    };

    job() = default;
    ~job() = default;

    using continuation_fn = std::function<void(status)>;

    inline status get_status() const { return current_status.load(std::memory_order::memory_order_acquire); }

    bool pending() const;
    bool running() const;
    bool done() const;
    bool succeeded() const;
    bool failed() const;

    void add_continuation(continuation_fn cont);

    natural_t id;

   private:
    friend class job_graph;

    std::atomic<status> current_status = status::PENDING;
    std::mutex cont_mutex;
    std::vector<continuation_fn> continuations;
  };

}  // namespace other

#endif  // OTHER_CORE_CORE_JOB_HPP