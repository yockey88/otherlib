/**
 * \file core/coroutine.cpp
 **/
#include "core/coroutine.hpp"

namespace other {

  bool task::job_awaiter::await_ready() const noexcept {
    return job_handle == nullptr || job_handle->done();
  }

}  // namespace other