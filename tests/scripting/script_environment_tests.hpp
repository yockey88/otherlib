/**
 * \file tests/scripting/script_environment_tests.hpp
 **/
#ifndef OTHER_TESTS_SCRIPTING_SCRIPT_ENVIRONMENT_TESTS_HPP
#define OTHER_TESTS_SCRIPTING_SCRIPT_ENVIRONMENT_TESTS_HPP

#include "other_test.hpp"

namespace other {

  class script_environment_tests : public other_test {
   public:
   protected:
    bool script_and_physics() const override { return true; }

    filepath main_other_dll = perform_tag_replacement("build/other-csharp/${build-config}/OtherCs.dll");
    filepath testing_dll = perform_tag_replacement("build/script-testing/${build-config}/DotnetTesting.dll");
  };

}  // namespace other

#endif  // OTHER_TESTS_SCRIPTING_SCRIPT_ENVIRONMENT_TESTS_HPP