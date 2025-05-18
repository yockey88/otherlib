/**
 * \file kernel/ref.hpp
 */
#include "kernel/ref.hpp"

#include <unordered_set>

namespace other {
  namespace {

    std::unordered_set<void*> refs;

  }  // namespace
  namespace detail {

    void RegisterReference(void* instance) {
      refs.insert(instance);
    }

    void RemoveReference(void* instance) {
      refs.erase(instance);
    }

    bool IsValidRef(void* instance) {
      return refs.find(instance) != refs.end();
    }

    size_t NumberOfLivingReferences() {
      return refs.size();
    }

  }  // namespace detail
}  // namespace other
