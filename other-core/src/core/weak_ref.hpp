/**
 * \file core/weak_ref.hpp
 **/
#ifndef OTHER_CORE_WEAK_REF_HPP
#define OTHER_CORE_WEAK_REF_HPP

namespace other {
  namespace detail {

    void register_reference(void* instance);
    void remove_reference(void* instance);
    bool is_valid_ref(void* instance);
    size_t num_living_references();

  }  // namespace detail

}  // namespace other

#endif  // OTHER_CORE_WEAK_REF_HPP