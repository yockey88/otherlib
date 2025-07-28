/**
 * \file bindings/native_logger.hpp
 **/
#ifndef OTHER_SCRIPTING_BINDINGS_NATIVE_LOGGER_HPP
#define OTHER_SCRIPTING_BINDINGS_NATIVE_LOGGER_HPP

#include "dotnet/native_string.hpp"

namespace other {

  class logger;

  enum native_log_level : int32_t {
    TRACE = 0,
    DEBUG,
    INFO,
    WARNING,
    ERR,
    CRITICAL,
  };

  namespace bindings {

    void native_log_message(logger* logger, native_string message, int32_t level);

  }  // namespace bindings
}  // namespace other

#endif  // OTHER_SCRIPTING_BINDINGS_NATIVE_LOGGER_HPP
