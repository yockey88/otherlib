/**
 * \file tests/scripting/action_tests.hpp
 **/
#ifndef OTHERLIB_TESTS_SCRIPTING_ACTION_TESTS_HPP
#define OTHERLIB_TESTS_SCRIPTING_ACTION_TESTS_HPP

#include "other_test.hpp"

namespace other {

  class action_tests : public other_test {
   public:
    action_tests() = default;
    virtual ~action_tests() = default;

    filepath other_dll_debug = "build/other-csharp/Debug/OtherCs.dll";
    filepath other_dll_release = "build/other-csharp/Release/OtherCs.dll";

    filepath testing_dll_debug = "build/script-testing/Debug/DotnetTesting.dll";
    filepath testing_dll_release = "build/script-testing/Release/DotnetTesting.dll";

    ref<assembly> dotnet_asm = nullptr;
    ref<assembly> testing_asm = nullptr;
  };

}  // namespace other

#endif  // OTHERLIB_TESTS_SCRIPTING_ACTION_TESTS_HPP