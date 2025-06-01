/**
 * \file thread/channel.hpp
 **/
#ifndef OTHERLIB_THREAD_CHANNEL_HPP
#define OTHERLIB_THREAD_CHANNEL_HPP

#include <condition_variable>
#include <mutex>
#include <queue>

#include "core/defines.hpp"
#include "core/logger.hpp"
#include "core/ref.hpp"
#include "core/ref_counted.hpp"
#include "core/scope.hpp"

namespace other {

  template <typename T>
  struct channel_queue : public ref_counted {
    virtual ~channel_queue() = default;

    std::queue<T> queue;
    std::mutex mutex;
    std::condition_variable condition;
  };

  template <typename T>
  class channel {
   public:
    channel() = default;
    channel(ref<channel_queue<T>> queue)
        : queue(queue) {}
    ~channel() {}

    ref<channel_queue<T>> queue;

    void push(T&& item) {
      OTHER_ASSERT(queue != nullptr, "Channel queue is invalid!");

      std::lock_guard lck(queue->mutex);
      queue->queue.push(std::forward<T>(item));
      queue->condition.notify_all();
    }

    bool empty() {
      std::lock_guard lck(queue->mutex);
      return queue->queue.empty();
    }

    opt<T> await_message(std::chrono::microseconds timeout = std::chrono::microseconds(1000)) {
      OTHER_ASSERT(queue != nullptr, "Awaiting message on a null queue!");

      std::unique_lock lck(queue->mutex);
      queue->condition.wait_for(lck, timeout, [&]() -> bool { return !queue->queue.empty(); });
      if (queue->queue.empty()) {
        return std::nullopt;
      }

      T item = std::move(queue->queue.front());
      queue->queue.pop();

      return std::move(item);
    }

    size_t size() const {
      return queue->queue.size();
    }

    static std::pair<scope<channel<T>>, scope<channel<T>>> make_channel(ref<channel_queue<T>> queue) {
      return {
        make_scope<channel>(queue),
        make_scope<channel>(queue),
      };
    }
  };

  template <typename T>
  channel(channel_queue<T>) -> channel<T>;

}  // namespace other

#endif  // OTHERLIB_THREAD_CHANNEL_HPP