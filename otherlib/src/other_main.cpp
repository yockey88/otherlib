/**
 * @file other_main.cpp
 * @brief Provides the main entry point for Other applications
 *
 * This file contains the main() function that all Other applications use.
 * It is compiled into a separate object file and linked by consumers.
 * Consumers should implement other_main() instead of main().
 */
#include "other.hpp"

#ifdef OTHER_ENVIRONMENT_WINDOWS
  #include <windows.h>
#endif

int main(int argc, char* argv[]) {
  return other::entry(argc, argv);
}
