/**
 * \file other.hpp
 **/
#ifndef OTHER_HPP
#define OTHER_HPP

#include "core/defines.hpp"
#include "core/logger.hpp"

#include "plugin/plugin.hpp"

#include "driver/driver.hpp"

namespace other {

  struct other_plugin_argv;

  namespace environment {

    int entry(int argc, char* argv[]);

  }  // namespace environment
}  // namespace other

#endif  // OTHER_HPP