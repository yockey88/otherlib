/**
 * \file scripting/dotnet_scripting_tests.hpp
 **/
#ifndef OTHER_TESTS_SCRIPTING_DOTNET_SCRIPTING_TESTS_HPP
#define OTHER_TESTS_SCRIPTING_DOTNET_SCRIPTING_TESTS_HPP

#include "other_test.hpp"

namespace other {

  class dotnet_scripting_tests : public other_test {
   public:
    void SetUp() {
      subsystem<arena>::get()->shutdown();
      subsystem<arena>::get();

      subsystem<scripting_environment>::get()->initialize_script_environment();
#ifdef OTHER_ENVIRONMENT_DEBUG
      dotnet_asm = subsystem<scripting_environment>::get()->load_dotnet_module(other_dll_debug.string());
      testing_asm = subsystem<scripting_environment>::get()->load_dotnet_module(testing_dll_debug.string());
#elif defined(OTHER_ENVIRONMENT_RELEASE)
      dotnet_asm = subsystem<scripting_environment>::get()->load_dotnet_module(other_dll_release.string());
      testing_asm = subsystem<scripting_environment>::get()->load_dotnet_module(testing_dll_release.string());
#else
  #error "Unknown build configuration!"
#endif
    }
    void TearDown() {
      subsystem<scripting_environment>::get()->unload_dotnet_module(testing_asm);
      subsystem<scripting_environment>::get()->unload_dotnet_module(dotnet_asm);
      testing_asm = nullptr;
      dotnet_asm = nullptr;
      subsystem<scripting_environment>::get()->shutdown_script_environment();

      subsystem<arena>::get()->shutdown();
    }

    filepath other_dll_debug = "build/other-csharp/Debug/OtherCs.dll";
    filepath other_dll_release = "build/other-csharp/Release/OtherCs.dll";

    filepath testing_dll_debug = "build/development-drivers/script-testing/csharp/Debug/DotnetTesting.dll";
    filepath testing_dll_release = "build/development-drivers/script-testing/csharp/Release/DotnetTesting.dll";

    ref<assembly> dotnet_asm = nullptr;
    ref<assembly> testing_asm = nullptr;
  };

}  // namespace other

#endif  // OTHER_TESTS_SCRIPTING_DOTNET_SCRIPTING_TESTS_HPP