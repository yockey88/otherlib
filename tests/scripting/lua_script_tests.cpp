/**
 * \file tests/scripting/lua_script_tests.cpp
 **/
#include "lua_script_tests.hpp"

#include "lua/sol_bridge.hpp"

namespace other {

  TEST_F(lua_script_tests, read_lua_table) {
    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Scripting environment subsystem is not initialized.");
    env->initialize_script_environment(environment->config);

    lua_script* test1 = env->load_lua_file("tests/test1.lua");
    ASSERT_TRUE(test1->is_valid());

    sol::table table1 = test1->get_symbol_as_table("table1");
    ASSERT_TRUE(table1.valid());

    std::string key1 = table1.get<std::string>("key1");
    ASSERT_EQ(key1, "value1");

    int key2 = table1.get<int>("key2");
    ASSERT_EQ(key2, 42);

    sol::table key3 = table1.get<sol::table>("key3");
    ASSERT_TRUE(key3.valid());
    std::vector<int> vec_key3;
    for (auto& pair : key3) {
      vec_key3.push_back(pair.second.as<int>());
    }
    ASSERT_EQ(vec_key3.size(), 5);
    ASSERT_EQ(vec_key3[0], 1);
    ASSERT_EQ(vec_key3[1], 2);
    ASSERT_EQ(vec_key3[2], 3);
    ASSERT_EQ(vec_key3[3], 4);
    ASSERT_EQ(vec_key3[4], 5);

    env->destroy_all_objects();
    env->shutdown_script_environment();
  }

  TEST_F(lua_script_tests, invalid_lua_file) {
    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Scripting environment subsystem is not initialized.");
    env->initialize_script_environment(environment->config);

    lua_script* invalid_script = env->load_lua_file("other-lua-interop/non_existent.lua");
    ASSERT_EQ(invalid_script, nullptr);

    env->destroy_all_objects();
    env->shutdown_script_environment();
  }

  TEST_F(lua_script_tests, call_lua_function) {
    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Scripting environment subsystem is not initialized.");
    env->initialize_script_environment(environment->config);

    lua_script* test1 = env->load_lua_file("tests/test1.lua");
    ASSERT_NE(test1, nullptr);
    ASSERT_TRUE(test1->is_valid());

    // Assuming test1.lua has a function defined as:
    // function add(a, b)
    //     return a + b
    // end

    int result = test1->call_function<int>("add", 5, 7);
    ASSERT_EQ(result, 12);

    env->destroy_all_objects();
    env->shutdown_script_environment();
  }

  TEST_F(lua_script_tests, lua_sandbox_test) {
    sol::state lua;
    lua.open_libraries(sol::lib::base);

    int32_t my_value = 42;
    auto native = lua.create_named_table("native_table");
    native["my_value"] = my_value;

    sol::environment sandbox_1(lua);
    {
      auto sandbox_table_1 = sandbox_1["native_table"];
      CORE_LOG_DEBUG("sandbox_table_1 my_value: {}", sandbox_table_1["my_value"].get<int32_t>());
      sandbox_table_1["my_value"] = 100;
    }

    sol::environment sandbox_2(lua);
    {
      auto sandbox_table_2 = sandbox_2["native_table"];
      CORE_LOG_DEBUG("sandbox_table_2 my_value: {}", sandbox_table_2["my_value"].get<int32_t>());
      sandbox_table_2["my_value"] = 200;
    }

    CORE_LOG_DEBUG("sandbox_table_1 my_value after modification: {}", sandbox_1["native_table"]["my_value"].get<int32_t>());
    CORE_LOG_DEBUG("sandbox_table_2 my_value after modification: {}", sandbox_2["native_table"]["my_value"].get<int32_t>());
  }

}  // namespace other