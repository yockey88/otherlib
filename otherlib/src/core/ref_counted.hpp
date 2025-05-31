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
        : views(0), ref_count(0) {}
    virtual ~ref_counted() = default;

    void view_increment() const;
    void view_decrement() const;

    void increment();
    void decrement();

    natural_t view_count() const;
    natural_t count() const;

   private:
    mutable std::atomic<natural_t> views;
    mutable std::atomic<natural_t> ref_count;
  };

}  // namespace other

#endif  // OTHER_CORE_REF_COUNTED_HPP
