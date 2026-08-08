/**
 * \file tests/core/version_tests.cpp
 **/
#include <format>
#include <fstream>
#include <string>

#include <gtest/gtest.h>

#include "core/version.hpp"

namespace other {

  /// VERSION is the release source of truth; a mismatch means it was bumped without reconfiguring
  TEST(version_tests, compiled_version_matches_version_file) {
    std::ifstream file("VERSION");
    ASSERT_TRUE(file.is_open()) << "VERSION not found; run the suite from the repo root";

    std::string text;
    std::getline(file, text);
    while (!text.empty() && (text.back() == '\r' || text.back() == ' ')) {
      text.pop_back();
    }

    EXPECT_EQ(text, OTHER_ENVIRONMENT_VERSION_STRING);
    EXPECT_EQ(std::format("{}.{}.{}", OTHER_ENVIRONMENT_VERSION_MAJOR, OTHER_ENVIRONMENT_VERSION_MINOR, OTHER_ENVIRONMENT_VERSION_PATCH),
              OTHER_ENVIRONMENT_VERSION_STRING);
  }

}  // namespace other
