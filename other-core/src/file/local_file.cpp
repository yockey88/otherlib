/**
 * \file file/local_file.cpp
 **/
#include "file/local_file.hpp"

namespace other {

  bool local_file::exists() const {
    return std::filesystem::exists(abs_path);
  }

  natural_t local_file::size() const {
    if (!std::filesystem::exists(abs_path)) {
      return 0;
    }
    return static_cast<natural_t>(std::filesystem::file_size(abs_path));
  }

}  // namespace other