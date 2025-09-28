/**
 * \file tests/serialization/serialization_tests.hpp
 **/
#ifndef OTHER_TESTS_SERIALIZATION_SERIALIZATION_TESTS_HPP
#define OTHER_TESTS_SERIALIZATION_SERIALIZATION_TESTS_HPP

#include "other_test.hpp"

namespace other {

  class serialization_tests : public other_test {
   public:
    void SetUp() {
      subsystem<arena>::get()->shutdown();
      subsystem<arena>::get();

      subsystem<scripting_environment>::get()->initialize_script_environment();
      dotnet_asm = subsystem<scripting_environment>::get()->load_dotnet_module("build/other-csharp/Debug/OtherCs.dll");
      testing_asm = subsystem<scripting_environment>::get()->load_dotnet_module("build/development-drivers/script-testing/csharp/Debug/DotnetTesting.dll");
    }
    void TearDown() {
      subsystem<scripting_environment>::get()->unload_dotnet_module(testing_asm);
      subsystem<scripting_environment>::get()->unload_dotnet_module(dotnet_asm);
      testing_asm = nullptr;
      dotnet_asm = nullptr;
      subsystem<scripting_environment>::get()->shutdown_script_environment();

      subsystem<arena>::get()->shutdown();
    }

    ref<assembly> dotnet_asm = nullptr;
    ref<assembly> testing_asm = nullptr;
  };

}  // namespace other

#endif  // OTHER_TESTS_SERIALIZATION_SERIALIZATION_TESTS_HPP