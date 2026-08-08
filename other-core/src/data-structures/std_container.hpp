/**
 * \file data-structures/std_containers.hpp
 **/
#ifndef OTHER_CORE_DATA_STRUCTURES_STD_CONTAINER_HPP
#define OTHER_CORE_DATA_STRUCTURES_STD_CONTAINER_HPP

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "memory/memory_resource.hpp"

namespace other {
  namespace ostd {

    template <typename T>
    using vector = ::std::vector<T, std_arena_allocator<T>>;
    template <typename T>
    using frame_vector = ::std::vector<T, std_frame_allocator<T>>;

    // undecided: migration effort + special string handling makes the payoff unclear
    // using string = ::std::basic_string<char, ::std::char_traits<char>, std_arena_allocator<char>>;

    template <typename K, typename V, typename Cmp = ::std::less<K>>
    using map = ::std::map<K, V, Cmp, std_arena_allocator<::std::pair<const K, V>>>;

    template <typename K, typename V, typename Hash = ::std::hash<K>, typename Eq = ::std::equal_to<K>>
    using unordered_map = ::std::unordered_map<K, V, Hash, Eq, std_arena_allocator<::std::pair<const K, V>>>;

  }  // namespace ostd
}  // namespace other

namespace ostd = other::ostd;

#endif  // OTHER_CORE_DATA_STRUCTURES_STD_CONTAINER_HPP