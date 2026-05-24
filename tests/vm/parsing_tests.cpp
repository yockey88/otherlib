/**
 * \file tests/vm/parsing_tests.cpp
 **/
#include "serialization/parser_combinators.hpp"

#include "vm/vm_tests.hpp"

namespace other {

  TEST_F(vm_tests, basic_oasm_parsing) {
    std::string source = R"(
  dumpx r1
  dumpx r2
  dumpx r3
  load r4, data.first_addr
  ret
    )";
  }

}  // namespace other