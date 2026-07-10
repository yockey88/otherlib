/**
 * \file tests/scripting/lua_script_tests.hpp
 **/
#ifndef OTHERLIB_TESTS_SCRIPTING_LUA_SCRIPT_TESTS_HPP
#define OTHERLIB_TESTS_SCRIPTING_LUA_SCRIPT_TESTS_HPP

#include "other_test.hpp"

namespace other {

  class lua_script_tests : public other_test {
   public:
    lua_script_tests() = default;
    virtual ~lua_script_tests() = default;

    bool script_and_physics() const override { return true; }
  };

}  // namespace other

#endif  // OTHERLIB_TESTS_SCRIPTING_LUA_SCRIPT_TESTS_HPP