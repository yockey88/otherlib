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
#include "core/profiler.hpp"
#include "core/ref.hpp"
#include "core/ref_counted.hpp"
#include "core/scope.hpp"
#include "core/time.hpp"

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
      PROFILE_SECTION("channel<T>::push");

      std::lock_guard lck(queue->mutex);
      queue->queue.push(std::forward<T>(item));
      queue->condition.notify_all();
    }

    bool empty() {
      std::lock_guard lck(queue->mutex);
      return queue->queue.empty();
    }

    /// non-blocking pop (wait_for has a ~1ms floor on windows)
    opt<T> try_pop() {
      OTHER_ASSERT(queue != nullptr, "Channel queue is invalid!");
      std::lock_guard lck(queue->mutex);
      if (queue->queue.empty()) {
        return std::nullopt;
      }
      T item = std::move(queue->queue.front());
      queue->queue.pop();
      return std::move(item);
    }

    opt<T> await_message(opt<microseconds> timeout = std::nullopt) {
      OTHER_ASSERT(queue != nullptr, "Awaiting message on a null queue!");
      PROFILE_SECTION("channel<T>::await_message");

      if (!timeout.has_value()) {
        std::unique_lock lck(queue->mutex);
        queue->condition.wait(lck, [&]() -> bool { return !queue->queue.empty(); });
        T item = std::move(queue->queue.front());
        queue->queue.pop();
        return std::move(item);
      }

      std::unique_lock lck(queue->mutex);
      queue->condition.wait_for(lck, *timeout, [&]() -> bool { return !queue->queue.empty(); });
      if (queue->queue.empty()) {
        return std::nullopt;
      }

      T item = std::move(queue->queue.front());
      queue->queue.pop();

      return std::move(item);
    }

    size_t size() const {
      std::lock_guard lck(queue->mutex);
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