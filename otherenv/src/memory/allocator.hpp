/**
 * \file memory/allocator.hpp
 **/
#ifndef OTHERENV_MEMORY_ALLOCATOR_HPP
#define OTHERENV_MEMORY_ALLOCATOR_HPP

namespace other {

  class Allocator {
   public:
    virtual ~Allocator() = default;
  };

}  // namespace other

#endif  // !OTHERENV_MEMORY_ALLOCATOR_HPP