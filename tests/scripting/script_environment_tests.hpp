/**
 * \file tests/scripting/script_environment_tests.hpp
 **/
#ifndef OTHER_TESTS_SCRIPTING_SCRIPT_ENVIRONMENT_TESTS_HPP
#define OTHER_TESTS_SCRIPTING_SCRIPT_ENVIRONMENT_TESTS_HPP

#include "other_test.hpp"

namespace other {

  class script_environment_tests : public other_test {
   protected:
    filepath main_other_dll_debug = "build/other-csharp/Debug/OtherCs.dll";
    filepath main_other_dll_release = "build/other-csharp/Release/OtherCs.dll";

    filepath testing_dll_debug = "build/development-drivers/script-testing/csharp/Debug/DotnetTesting.dll";
    filepath testing_dll_release = "build/development-drivers/script-testing/csharp/Release/DotnetTesting.dll";
  };

}  // namespace other

#endif  // OTHER_TESTS_SCRIPTING_SCRIPT_ENVIRONMENT_TESTS_HPP