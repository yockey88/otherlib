/**
 * \file scratch_tests.cpp
 **/
#include "./other_test.hpp"

namespace other {

  TEST(test0, simple_string_split_test) {
    std::string name = "Namespace.Class.Method()";
    auto last_dot = name.find_last_of('.');
    OTHER_ASSERT(last_dot != std::string::npos, "Inalid .NET class name in pass resolver!");

    auto cname = name.substr(0, last_dot);
    auto mname = name.substr(last_dot + 1, name.size() - (last_dot + 1));

    EXPECT_EQ(cname, "Namespace.Class");
    EXPECT_EQ(mname, "Method()");
  }

}  // namespace other