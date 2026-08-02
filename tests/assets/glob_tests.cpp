/**
 * \file tests/assets/glob_tests.cpp
 *
 * contract under test: glob_set include/exclude matching, subtree pruning
 *  (the rebuild-loop safety mechanism), and order-independent content hashing.
 **/
#include <gtest/gtest.h>

#include "file/glob.hpp"

#include "other_test.hpp"

namespace other {

  class glob_tests : public other_test {};

  namespace {

    glob_set sdk_compile_set() {
      glob_set set;
      set.include("**/*.cs");
      set.exclude({ "bin/**", "obj/**", ".*/**" });
      return set;
    }

  }  // namespace

  TEST_F(glob_tests, double_star_matches_nested) {
    const glob_set set = sdk_compile_set();
    EXPECT_TRUE(set.matches("a.cs"));
    EXPECT_TRUE(set.matches("a/b/c.cs"));
    EXPECT_FALSE(set.matches("a/b/c.txt"));
  }

  TEST_F(glob_tests, exclude_wins_over_include) {
    const glob_set set = sdk_compile_set();
    EXPECT_FALSE(set.matches("bin/Debug/generated.cs"));
    EXPECT_FALSE(set.matches("obj/Debug/temp.cs"));
    EXPECT_FALSE(set.matches(".vs/cache.cs"));
    EXPECT_TRUE(set.matches("assets/scripts/sim.cs"));
  }

  TEST_F(glob_tests, may_contain_prunes_bin_obj_dotdirs) {
    const glob_set set = sdk_compile_set();
    EXPECT_FALSE(set.may_contain("bin"));
    EXPECT_FALSE(set.may_contain("bin/Debug"));
    EXPECT_FALSE(set.may_contain("obj"));
    EXPECT_FALSE(set.may_contain(".git"));
    EXPECT_TRUE(set.may_contain("assets"));
    EXPECT_TRUE(set.may_contain("assets/scripts"));
  }

#ifdef OTHER_ENVIRONMENT_WINDOWS
  TEST_F(glob_tests, windows_case_insensitive_match) {
    const glob_set set = sdk_compile_set();
    EXPECT_TRUE(set.matches("Assets/Scripts/Sim.CS"));
    EXPECT_FALSE(set.matches("BIN/Debug/generated.cs"));
  }
#endif

  TEST_F(glob_tests, content_hash_order_independent) {
    glob_set a;
    a.include("**/*.cs");
    a.include("**/*.lua");
    a.exclude({ "bin/**", "obj/**" });

    glob_set b;
    b.include("**/*.lua");
    b.include("**/*.cs");
    b.exclude({ "obj/**", "bin/**" });

    EXPECT_EQ(a.content_hash(), b.content_hash());

    glob_set c = sdk_compile_set();
    EXPECT_NE(a.content_hash(), c.content_hash());
  }

}  // namespace other
