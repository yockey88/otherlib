/**
 * \file core/ref.hpp
 */
#include "core/ref.hpp"

#include <unordered_set>

namespace other {
  namespace {

    std::unordered_set<void*> refs;

  }  // namespace
  namespace detail {

    void register_reference(void* instance) {
      refs.insert(instance);
    }

    void remove_reference(void* instance) {
      refs.erase(instance);
    }

    bool is_valid_ref(void* instance) {
      return refs.find(instance) != refs.end();
    }

    size_t num_living_references() {
      return refs.size();
    }

  }  // namespace detail
}  // namespace other
