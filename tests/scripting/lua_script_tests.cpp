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

    lua_script* test1 = env->load_lua_file("resources/lua/test1.lua");
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

    env->shutdown_script_environment();
  }

  TEST_F(lua_script_tests, invalid_lua_file) {
    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Scripting environment subsystem is not initialized.");
    env->initialize_script_environment(environment->config);

    lua_script* invalid_script = env->load_lua_file("resources/lua/non_existent.lua");
    ASSERT_FALSE(invalid_script->is_valid());

    env->shutdown_script_environment();
  }

  TEST_F(lua_script_tests, call_lua_function) {
    auto* env = subsystem<scripting_environment>::get();
    OTHER_ASSERT(env != nullptr, "Scripting environment subsystem is not initialized.");
    env->initialize_script_environment(environment->config);

    lua_script* test1 = env->load_lua_file("resources/lua/test1.lua");
    ASSERT_TRUE(test1->is_valid());

    // Assuming test1.lua has a function defined as:
    // function add(a, b)
    //     return a + b
    // end

    int result = test1->call_function<int>("add", 5, 7);
    ASSERT_EQ(result, 12);

    env->shutdown_script_environment();
  }

}  // namespace other