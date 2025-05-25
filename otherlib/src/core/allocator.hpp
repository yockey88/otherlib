/**
 * \file core/allocator.hpp
 **/
#ifndef OTHER_CORE_MEMORY_ALLOCATOR_HPP
#define OTHER_CORE_MEMORY_ALLOCATOR_HPP

namespace other {

  class allocator {
   public:
    virtual ~allocator() = default;
  };

}  // namespace other

#endif  // OTHER_CORE_MEMORY_ALLOCATOR_HPP