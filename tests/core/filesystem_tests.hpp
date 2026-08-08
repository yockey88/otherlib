/**
 * \file tests/core/filesystem_tests.hpp
 **/
#ifndef OTHER_TESTS_CORE_FILESYSTEM_TESTS_HPP
#define OTHER_TESTS_CORE_FILESYSTEM_TESTS_HPP

#include <fstream>

#include "file/filesystem.hpp"

#include "other_test.hpp"

namespace other {

  /// hermetic on-disk tree: <temp>/{root-file.txt, sub/nested.txt, sub/deep/leaf.lua}
  class filesystem_tests : public other_test {
   protected:
    static constexpr std::string_view kMountName = "fstest";

    filepath temp_root;

    void SetUp() override {
      other_test::SetUp();

      temp_root = std::filesystem::temp_directory_path() / "other-filesystem-tests";
      std::filesystem::remove_all(temp_root);
      std::filesystem::create_directories(temp_root / "sub" / "deep");
      write_test_file(temp_root / "root-file.txt", "root contents");
      write_test_file(temp_root / "sub" / "nested.txt", "nested contents");
      write_test_file(temp_root / "sub" / "deep" / "leaf.lua", "-- leaf");
    }

    void TearDown() override {
      /// subsystems (and their file handles) must be torn down before deleting the tree
      other_test::TearDown();
      std::error_code ec;
      std::filesystem::remove_all(temp_root, ec);
    }

    static void write_test_file(const filepath& path, std::string_view contents) {
      std::ofstream out(path);
      out << contents;
    }
  };

}  // namespace other

#endif  // OTHER_TESTS_CORE_FILESYSTEM_TESTS_HPP
