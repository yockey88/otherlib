/**
 * \file vm/diagnostics/source_map.hpp
 **/
#ifndef OTHERLIB_VM_DIAGNOSTICS_SOURCE_MAP_HPP
#define OTHERLIB_VM_DIAGNOSTICS_SOURCE_MAP_HPP


namespace other {

  class source_map {
   public:
    source_map() = default;
    ~source_map() = default;

    natural_t add_source(const std::string_view name, const std::string_view text);

    std::string_view get_source_text(natural_t id);
    std::string_view get_source_name(natural_t id);

   private:
    struct source {
      std::string name;
      std::string text;
    };
    /// std: reachable from the static vm::diagnostics engine
    std::map<natural_t, source> sources;
  };

}  // namespace other

#endif  // OTHERLIB_VM_DIAGNOSTICS_SOURCE_MAP_HPP