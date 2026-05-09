/**
 * \file core/job.cpp
 **/
#include "core/job.hpp"

namespace other {

  job::status job::get_status() const {
    return current_status.load(std::memory_order_acquire);
  }

  bool job::pending() const {
    return get_status() <= status::QUEUED;
  }

  bool job::running() const {
    return get_status() == status::RUNNING;
  }

  bool job::done() const {
    auto s = get_status();
    return s == status::COMPLETED || s == status::FAILED || s == status::CANCELLED;
  }

  bool job::succeeded() const {
    return get_status() == status::COMPLETED;
  }

  bool job::failed() const {
    return get_status() == status::FAILED;
  }

}  // namespace other