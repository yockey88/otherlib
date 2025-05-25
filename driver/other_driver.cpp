/**
 * \file other_driver.cpp
 **/
#include <iostream>
#include <print>

#include "other.hpp"

#ifdef OTHER_ENVIRONMENT_WINDOWS
  #ifdef HAS_SEH_EXCEPTIONS
    #include <windows.h>
int main(int argc, char* argv[]) {
  __try {
    return other::environment::entry(argc, argv);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    std::println(std::cerr, "An exception occurred in the Other Environment driver.");
    return -1;
  }
}
  #else
int main(int argc, char* argv[]) {
  return other::environment::entry(argc, argv);
}
  #endif
#else
int main(int argc, char* argv[]) {
  return other::environment::entry(argc, argv);
}
#endif
