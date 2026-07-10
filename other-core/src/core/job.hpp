/**
 * \file core/job.hpp
 **/
#ifndef OTHER_CORE_CORE_JOB_HPP
#define OTHER_CORE_CORE_JOB_HPP

#include <atomic>
#include <functional>
#include <mutex>

#include "core/defines.hpp"
#include "core/ref_counted.hpp"
#include "data-structures/std_container.hpp"

namespace other {

  class OTHER_CLASS job : public ref_counted {
   public:
    enum class priority {
      LOW,
      MEDIUM,
      HIGH,
      CRITICAL,

      NUM_JOB_PRIORITIES,
      INVALID_JOB_PRIORITY = NUM_JOB_PRIORITIES,
    };
    enum class status {
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
    enum class affinity {
      ANY_THREAD,
      MAIN_THREAD,
      WORKER_THREAD,

      NUM_JOB_AFFINITIES,
      INVALID_JOB_AFFINITY = NUM_JOB_AFFINITIES,
    };
    struct descriptor {
      std::string name = "Unnamed Job";
      priority priority = priority::MEDIUM;
      affinity thread_affinity = affinity::MAIN_THREAD;
    };

    job() = default;
    ~job() = default;

    using continuation_fn = std::function<void(status)>;

    status get_status() const;
    bool pending() const;
    bool running() const;
    bool done() const;
    bool succeeded() const;
    bool failed() const;

    void add_continuation(continuation_fn cont);

    natural_t id;
    std::atomic<status> current_status = status::PENDING;

   private:
    std::mutex cont_mutex;
    ostd::vector<continuation_fn> continuations;
  };

}  // namespace other

#endif  // OTHER_CORE_CORE_JOB_HPP