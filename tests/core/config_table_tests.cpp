/**
 * \file core/config_table_test.cpp
 **/
#include "core/config_table_tests.hpp"

namespace other {

  TEST_F(config_table_tests, load_from_file) {
    const std::string test_config =
      R"(
    [section1]
    key1 = "value1"
    [section2]
    key2 = 42
    key4 = true
    [section3]
    key3 = [1, 2, 3]
  )";

    config_table cfg = config_table::load_from_source(test_config);
    ASSERT_TRUE(cfg.valid);
    ASSERT_TRUE(cfg.has_path("section1.key1"));

    EXPECT_EQ(cfg.get_value<std::string>("section1.key1", "incorrect"), "value1");
    EXPECT_EQ(cfg.get_value<int>("section2.key2", 0), 42);
    EXPECT_EQ(cfg.get_value<bool>("section2.key4", false), true);

    auto key3 = cfg.get_value<std::vector<int>>("section3.key3", {});
    EXPECT_EQ(key3.size(), 3);
    EXPECT_EQ(key3[0], 1);
    EXPECT_EQ(key3[1], 2);
    EXPECT_EQ(key3[2], 3);
  }

}  // namespace other