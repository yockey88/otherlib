/**
 * \file callback_tests.cpp
 **/
#include "core/ref.hpp"

#include "scripting/actions/callback.hpp"
#include "scripting/interface_registry.hpp"

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

  /// regression: assembly hot reload unregisters an assembly's named callbacks on the unload
  /// half so the fresh assembly can register the same names again (previously fatal)
  TEST_F(action_tests, named_callback_reregistration_after_unregister) {
    interface_registry registry;

    int calls = 0;
    registry.register_named_callback("Test.Callback", make_ref<native_callback<void>>([&calls]() { calls += 1; }));
    registry.invoke_callback("Test.Callback");
    ASSERT_EQ(calls, 1);

    registry.unregister_named_callback("Test.Callback");
    registry.invoke_callback("Test.Callback");  /// unknown callback warns and no-ops
    ASSERT_EQ(calls, 1);

    registry.register_named_callback("Test.Callback", make_ref<native_callback<void>>([&calls]() { calls += 10; }));
    registry.invoke_callback("Test.Callback");
    ASSERT_EQ(calls, 11);
  }

  /// dotnet callbacks need the headless profile: base action_tests runs the minimal profile,
  ///  which never initializes the scripting environment or loads the DotnetTesting assembly
  class dotnet_action_tests : public action_tests {
   protected:
    bool script_and_physics() const override { return true; }
  };

  TEST_F(dotnet_action_tests, dotnet_callback_test) {
    auto* env = subsystem<scripting_environment>::get();
    ASSERT_NE(env, nullptr);

    integer_t obj_id = env->create_object("MyObject");
    ASSERT_GE(obj_id, 0);

    env->attach_dotnet_object(obj_id, "TestObject");
    script_object* script_obj = env->get_object(obj_id);
    ASSERT_NE(script_obj, nullptr);
    ASSERT_NE(script_obj->dotnet_object, nullptr);

    {
      ref<callback> callback_fn = make_ref<dotnet_callback>(script_obj->dotnet_object, "Add");
      ASSERT_NE(callback_fn, nullptr);

      int ret = callback_fn->call<int>(10, 32);
      ASSERT_EQ(ret, 42);
    }

    env->detach_dotnet_object(obj_id);
    env->destroy_object(obj_id);
  }

  TEST_F(action_tests, lua_callback_test) {
    // GTEST_SKIP() << "Lua callback implementation needs to be redesigned";

    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Scripting environment subsystem is not initialized.");

    lua_script* test1 = env->load_lua_file("tests/test1.lua");
    ASSERT_TRUE(test1->is_valid());

    {
      ref<callback> callback_fn = make_ref<lua_callback>(test1, "add");
      ASSERT_NE(callback_fn, nullptr);

      int ret = callback_fn->call<int>(15, 27);
      ASSERT_EQ(ret, 42);
    }
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