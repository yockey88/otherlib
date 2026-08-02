/**
 * \file core/ref_counted.hpp
 */
#ifndef OTHER_CORE_REF_COUNTED_HPP
#define OTHER_CORE_REF_COUNTED_HPP

#include <atomic>

#include "core/defines.hpp"

namespace other {

  class ref_counted {
   public:
    ref_counted()
        : views(1), ref_count(0) {}
    virtual ~ref_counted() = default;

    void view_increment() const;
    /// returns the view count AFTER the operation; `view_decrement() == 0` means the
    ///  caller released the last view share and owns freeing the storage
    natural_t view_decrement() const;

    /// both return the reference count AFTER the operation, so `decrement() == 0`
    ///  means the caller released the last reference and owns destruction
    natural_t increment();
    natural_t decrement();

    /// increments the reference count only if it is currently nonzero
    /// false return means object is expired (weak_ref::lock uses this to refuse resurrection)
    bool try_increment();

    natural_t view_count() const;
    natural_t count() const;

   private:
    mutable std::atomic<natural_t> views;
    mutable std::atomic<natural_t> ref_count;
  };

}  // namespace other

#endif  // OTHER_CORE_REF_COUNTED_HPP
