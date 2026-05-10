/**
 * \file callback_tests.cpp
 **/
#include "core/ref.hpp"

#include "scripting/actions/callback.hpp"

#include "action_tests.hpp"

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
      native_callback ncallback_fn = native_callback(&test_func1);
      callback* callback_ptr = &ncallback_fn;
      ASSERT_NE(callback_ptr, nullptr);

      callback_ptr->call();
    }

    {
      ref<callback> callback_fn = make_ref<native_callback<int>>(&test_func2);
      ASSERT_NE(callback_fn, nullptr);

      int ret = callback_fn->call<int>();
      ASSERT_EQ(ret, 42);
    }

    {
      ref<callback> callback_fn = make_ref<native_callback<std::string>>([]() -> std::string {
        CORE_LOG_DEBUG("Lambda called for string return type");
        return "Hello, world!";
      });
      ASSERT_NE(callback_fn, nullptr);

      std::string ret = callback_fn->call<std::string>();
      ASSERT_EQ(ret, "Hello, world!");
    }
  }

  TEST_F(action_tests, simple_native_callback_test_with_parameters) {
    ref<callback> callback_fn = make_ref<native_callback<float, int, float, std::string>>(&test_func3);
    ASSERT_NE(callback_fn, nullptr);

    using namespace std::string_literals;
    float ret = callback_fn->call<float>(10, 3.2, "test"s);
    ASSERT_EQ(ret, 6.7f);
  }

  TEST_F(action_tests, lambda_native_callback_test) {
    ref<callback> callback_fn = make_ref<native_callback<double, int, float>>([](int a, float b) -> double {
      CORE_LOG_DEBUG("Lambda called with a={}, b={}", a, b);
      return a + b;
    });
    ASSERT_NE(callback_fn, nullptr);

    double ret = callback_fn->call<double>(5, 3.2f);
    ASSERT_NEAR(ret, 8.2, 1e-6);
  }

  TEST_F(action_tests, dotnet_callback_test) {
    GTEST_SKIP() << "Dotnet callback implementation needs to be redesigned";

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
      ref<callback> callback_fn = make_ref<dotnet_callback>(script_obj->dotnet_object, "Add");
      ASSERT_NE(callback_fn, nullptr);

      int ret = callback_fn->call<int>(10, 32);
      ASSERT_EQ(ret, 42);
    }

    env->detach_dotnet_object(obj_id);

    subsystem<scripting_environment>::get()->unload_dotnet_module(testing_asm);
    subsystem<scripting_environment>::get()->unload_dotnet_module(dotnet_asm);
    testing_asm = nullptr;
    dotnet_asm = nullptr;
    subsystem<scripting_environment>::get()->destroy_all_objects();
    subsystem<scripting_environment>::get()->shutdown_script_environment();

    subsystem<arena>::get()->shutdown();
  }

  TEST_F(action_tests, lua_callback_test) {
    // GTEST_SKIP() << "Lua callback implementation needs to be redesigned";

    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Scripting environment subsystem is not initialized.");
    env->initialize_script_environment(environment->config);

    lua_script* test1 = env->load_lua_file("tests/test1.lua");
    ASSERT_TRUE(test1->is_valid());

    {
      ref<callback> callback_fn = make_ref<lua_callback>(test1, "add");
      ASSERT_NE(callback_fn, nullptr);

      int ret = callback_fn->call<int>(15, 27);
      ASSERT_EQ(ret, 42);
    }

    env->destroy_all_objects();
    env->shutdown_script_environment();
  }

  TEST_F(action_tests, lua_callback_test_with_sol_function) {
    // GTEST_SKIP() << "Lua callback implementation needs to be redesigned";

    sol::state lua_state;
    lua_state.open_libraries(sol::lib::base);

    lua_state.script(R"(
      function add(a, b)
        return a + b
      end
    )");

    sol::function add_func = lua_state["add"];
    ASSERT_TRUE(add_func.valid());

    {
      ref<callback> callback_fn = make_ref<lua_callback>(add_func);
      ASSERT_NE(callback_fn, nullptr);

      int ret = callback_fn->call<int>(20, 22);
      ASSERT_EQ(ret, 42);
    }
  }

}  // namespace other