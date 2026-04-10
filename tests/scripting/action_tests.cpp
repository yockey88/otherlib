/**
 * \file tests/scripting/action_tests.cpp
 **/
#include "action_tests.hpp"

#include "core/defines.hpp"

#include "scripting/actions/action.hpp"
#include "scripting/actions/callback.hpp"

#include "gtest/gtest.h"

namespace other {

  void test_func1() {
    CORE_LOG_DEBUG("test_func1 called");
  }

  int test_func2() {
    CORE_LOG_DEBUG("test_func2 called");
    return 42;
  }

  float test_func3(int a, float b, std::string c) {
    CORE_LOG_DEBUG("test_func3 called with a={}, b={}, c={}", a, b, c);
    return 6.7f;
  }

  TEST_F(action_tests, simple_native_callback_test_no_parameters) {
    {
      scope<callback> callback_fn = make_scope<native_callback<void>>(&test_func1);
      ASSERT_NE(callback_fn, nullptr);

      callback_fn->call({});
    }

    {
      scope<callback> callback_fn = make_scope<native_callback<int>>(&test_func2);
      ASSERT_NE(callback_fn, nullptr);

      value ret = callback_fn->call({});
      ASSERT_EQ(ret.type(), value_type::INT32);
      ASSERT_EQ((int)ret, 42);
    }
  }

  TEST_F(action_tests, simple_native_callback_test_with_parameters) {
    scope<callback> callback_fn = make_scope<native_callback<float, int, float, std::string>>(&test_func3);
    ASSERT_NE(callback_fn, nullptr);

    using namespace std::string_literals;
    std::vector<value> args = { value(10), value(3.2f), value("test"s) };
    ASSERT_EQ(args.size(), 3);
    ASSERT_EQ(args[0].type(), value_type::INT32);
    ASSERT_EQ(args[1].type(), value_type::FLOAT);
    ASSERT_EQ(args[2].type(), value_type::STRING);
    std::string arg2_str = args[2];
    ASSERT_EQ(arg2_str, "test");

    value ret = callback_fn->call(args);
    ASSERT_EQ(ret.type(), value_type::FLOAT);
    ASSERT_EQ((float)ret, 6.7f);
  }

  TEST_F(action_tests, lambda_native_callback_test) {
    scope<callback> callback_fn = make_scope<native_callback<double, int, float>>([](int a, float b) -> double {
      CORE_LOG_DEBUG("Lambda called with a={}, b={}", a, b);
      return a + b;
    });
    ASSERT_NE(callback_fn, nullptr);

    std::vector<value> args = { value(5), value(3.2f) };
    value ret = callback_fn->call(args);
    ASSERT_EQ(ret.type(), value_type::DOUBLE);
    ASSERT_NEAR((double)ret, 8.2, 1e-6);
  }

  TEST_F(action_tests, dotnet_callback_test) {
    // GTEST_SKIP() << "Dotnet callback implementation needs to be redesigned";

    subsystem<arena>::get()->shutdown();
    subsystem<arena>::get();

    subsystem<scripting_environment>::get()->initialize_script_environment(environment->config);
#ifdef OTHER_ENVIRONMENT_DEBUG
    dotnet_asm = subsystem<scripting_environment>::get()->load_dotnet_module(other_dll_debug.string());
    testing_asm = subsystem<scripting_environment>::get()->load_dotnet_module(testing_dll_debug.string());
#elif defined(OTHER_ENVIRONMENT_RELEASE)
    dotnet_asm = subsystem<scripting_environment>::get()->load_dotnet_module(other_dll_release.string());
    testing_asm = subsystem<scripting_environment>::get()->load_dotnet_module(testing_dll_release.string());
#else
  #error "Unknown build configuration!"
#endif

    auto* env = subsystem<scripting_environment>::get();
    integer_t obj_id = env->create_object("MyObject");

    env->attach_dotnet_object(obj_id, "TestObject");
    script_object* script_obj = env->get_object(obj_id);

    {
      scope<callback> callback_fn = make_scope<dotnet_callback<int, int, int>>(script_obj->dotnet_object, "Add");
      ASSERT_NE(callback_fn, nullptr);

      std::vector<value> args = { value(10), value(32) };
      value ret = callback_fn->call(args);
      ASSERT_EQ(ret.type(), value_type::INT32);
      ASSERT_EQ((int)ret, 42);
    }

    env->detach_dotnet_object(obj_id);

    subsystem<scripting_environment>::get()->unload_dotnet_module(testing_asm);
    subsystem<scripting_environment>::get()->unload_dotnet_module(dotnet_asm);
    testing_asm = nullptr;
    dotnet_asm = nullptr;
    subsystem<scripting_environment>::get()->shutdown_script_environment();

    subsystem<arena>::get()->shutdown();
  }

  TEST_F(action_tests, lua_callback_test) {
    // GTEST_SKIP() << "Lua callback implementation needs to be redesigned";

    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Scripting environment subsystem is not initialized.");
    env->initialize_script_environment(environment->config);

    lua_script* test1 = env->load_lua_file("resources/lua/test1.lua");
    ASSERT_TRUE(test1->is_valid());

    {
      scope<callback> callback_fn = make_scope<lua_callback<int, int, int>>(test1, "add");
      ASSERT_NE(callback_fn, nullptr);

      std::vector<value> args = { value(15), value(27) };
      value ret = callback_fn->call(args);
      ASSERT_EQ(ret.type(), value_type::INT32);
      ASSERT_EQ((int)ret, 42);
    }

    env->shutdown_script_environment();
  }

}  // namespace other