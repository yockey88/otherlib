/**
 * \file kernel/ref_counted.hpp
 */
#ifndef OTHERENV_CORE_REF_COUNTED_HPP
#define OTHERENV_CORE_REF_COUNTED_HPP

#include <atomic>

namespace other {

  class RefCounted {
   public:
    RefCounted()
        : views(0), count(0) {}
    virtual ~RefCounted() = default;

    void ViewIncrement() const;
    void ViewDecrement() const;

    void Increment();
    void Decrement();

    uint64_t ViewCount() const;
    uint64_t Count() const;

   private:
    mutable std::atomic<uint64_t> views;
    mutable std::atomic<uint64_t> count;
  };

}  // namespace other

#endif  // !OTHER_ENGINE_REF_COUNTED_HPP
