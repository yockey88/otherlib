/**
 * \file file/path_resolution.hpp
 */
#ifndef OTHER_CORE_FILE_PATH_RESOLUTION_HPP
#define OTHER_CORE_FILE_PATH_RESOLUTION_HPP

namespace other {

  enum class mount_scope : uint8_t {
    ENGINE,
    PROJECT,
    PACK,
  };

  struct resolved_path {
    std::string mount_name;
    mount_scope scope = mount_scope::ENGINE;

    ostd::vector<std::string> relative_path_components;
    std::string file_name;
    std::string extension;

    bool is_valid() const { return !mount_name.empty(); }
  };

  struct resolved_file {
    filepath absolute_path;
    std::string virtual_path;
  };

}  // namespace other

#endif  // OTHER_CORE_FILE_PATH_RESOLUTION_HPP